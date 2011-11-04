/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "mpl-app-bookmark-manager.h"

/**
 * SECTION:mpl-app-bookmark-manager
 * @short_description: Application bookmark manager.
 * @Title: MplAppBookmarkManager
 *
 * #MplAppBookmarkManager manages user-bookmarked applications, as shown, for
 *  example, in the myzone and applications panels. The ::bookmarks-changed
 *  signal is emitted when the bookmarks have changed.
 */

G_DEFINE_TYPE (MplAppBookmarkManager, mpl_app_bookmark_manager, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_APP_BOOKMARK_MANAGER, MplAppBookmarkManagerPrivate))

typedef struct _MplAppBookmarkManagerPrivate MplAppBookmarkManagerPrivate;

struct _MplAppBookmarkManagerPrivate {
  gchar *path;
  GFileMonitor *monitor;
  guint save_idle_id;
  GList *uris;
  GHashTable *monitors_hash;
};

#define APP_BOOKMARK_FILENAME "favourite-apps"
#define APP_BOOKMARK_REMOVED_FILENAME APP_BOOKMARK_FILENAME ".removed"
#define APP_BOOKMARK_REMOVAL_TIMOUT_S 10

enum
{
  BOOKMARKS_CHANGED,
  LAST_SIGNAL
};

static void mpl_app_bookmark_manager_idle_save (MplAppBookmarkManager *manager);

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct {
  MplAppBookmarkManager *self;
  gchar                 *filename;
} BookmarkRemovalData;

static GList *
_list_pending_removals (MplAppBookmarkManager *self,
                        gboolean               delete_removals)
{
  GDir *dir;
  const gchar *entry;
  gchar *filename = NULL;
  gchar *uri = NULL;
  const gchar *prefix = APP_BOOKMARK_REMOVED_FILENAME ".";
  GList *list = NULL;
  GError *error = NULL;

  dir = g_dir_open (g_get_user_data_dir (), 0, &error);
  if (error)
  {
    g_warning (G_STRLOC ": %s", error->message);
    g_clear_error (&error);
    return NULL;
  }

  while (NULL != (entry = g_dir_read_name (dir)))
  {
    if (!g_str_has_prefix (entry, prefix))
      continue;

    filename = g_build_filename (g_get_user_data_dir (),
                                 entry,
                                 NULL);
    g_file_get_contents (filename, &uri, NULL, &error);
    if (error)
    {
      g_warning (G_STRLOC ": %s", error->message);
      g_clear_error (&error);
    }

    if (delete_removals)
    {
      if (0 != g_unlink (filename))
        g_warning (G_STRLOC ": could not delete file '%s'", filename);
    }

    g_free (filename);

    if (uri)
      list = g_list_prepend (list, uri);
  }
  g_dir_close (dir);

  return list;
}

static gboolean
_bookmark_removal_cb (BookmarkRemovalData *data)
{
  gchar *uri = NULL;
  GError *error = NULL;

  g_debug ("%s() %s", __FUNCTION__, data->filename);

  g_file_get_contents (data->filename, &uri, NULL, &error);
  if (error)
  {
    g_warning (G_STRLOC ": Error reading file '%s': %s",
                data->filename,
                error->message);
    g_clear_error (&error);
  } else {
    /* Actually remove the bookmark if the desktop file does not exist. */
    char *desktop_file = g_filename_from_uri (uri, NULL, &error);
    if (error)
    {
      g_warning (G_STRLOC ": Error converting URI to filename '%s': %s",
                 uri,
                 error->message);
      g_clear_error (&error);
    } else {
      if (!g_file_test (desktop_file, G_FILE_TEST_EXISTS))
        mpl_app_bookmark_manager_remove_uri (data->self, uri);
    }
    g_free (desktop_file);
  }

  if (0 != g_unlink (data->filename))
    g_warning (G_STRLOC ": could not delete file '%s'", data->filename);

  g_free (uri);
  g_free (data->filename);
  g_free (data);

  return FALSE;
}

static void
_queue_bookmark_removal (MplAppBookmarkManager  *self,
                         const gchar            *uri)
{
  gchar *filename = NULL;
  guint i = 0;
  GError *error = NULL;

  do {
    g_free (filename);
    filename = g_strdup_printf ("%s%c%s.%d",
                                g_get_user_data_dir (),
                                G_DIR_SEPARATOR,
                                APP_BOOKMARK_REMOVED_FILENAME,
                                i++);
  } while (g_file_test (filename, G_FILE_TEST_EXISTS));

  g_file_set_contents (filename, uri, -1, &error);
  if (error)
  {
    g_warning (G_STRLOC ": Error writing file '%s': %s",
                filename,
                error->message);
    g_clear_error (&error);
    g_free (filename);
  } else {
    BookmarkRemovalData *data = g_new0 (BookmarkRemovalData, 1);
    data->self = self;
    data->filename = filename;
    g_timeout_add_seconds (APP_BOOKMARK_REMOVAL_TIMOUT_S,
                           (GSourceFunc) _bookmark_removal_cb,
                           data);
  }
}

static void
_bookmark_desktop_file_changed_cb (GFileMonitor      *monitor,
                                   GFile             *file,
                                   GFile             *other_file,
                                   GFileMonitorEvent event,
                                   gpointer          userdata)
{
  MplAppBookmarkManager *self = (MplAppBookmarkManager *)userdata;
  GList *pending_removals;
  gchar *uri;

  /* Only care for removed apps that are bookmarked. */
  if (event != G_FILE_MONITOR_EVENT_DELETED)
    return;

  uri = g_file_get_uri (file);

  /* Filter out multiple notifications. */
  pending_removals = _list_pending_removals (self, FALSE);
  if (!g_list_find_custom (pending_removals, uri, (GCompareFunc) g_strcmp0))
  {
    _queue_bookmark_removal (self, uri);
  }

  g_list_foreach (pending_removals, (GFunc) g_free, NULL);
  g_list_free (pending_removals);
  g_free (uri);
}

static void
mpl_app_bookmark_manager_dispose (GObject *object)
{
  MplAppBookmarkManagerPrivate *priv = GET_PRIVATE (object);
  GList *l;

  if (priv->monitor)
  {
    g_file_monitor_cancel (priv->monitor);
    g_object_unref (priv->monitor);
    priv->monitor = NULL;
  }

  if (priv->monitors_hash)
  {
    g_hash_table_destroy (priv->monitors_hash);
    priv->monitors_hash = NULL;
  }

  if (priv->uris)
  {
    if (priv->save_idle_id > 0)
    {
      g_source_remove (priv->save_idle_id);
      mpl_app_bookmark_manager_save ((MplAppBookmarkManager *)object);
    }

    for (l = priv->uris; l; l = g_list_delete_link (l, l))
    {
      g_free ((gchar *)l->data);
    }

    priv->uris = NULL;
  }

  G_OBJECT_CLASS (mpl_app_bookmark_manager_parent_class)->dispose (object);
}

static void
mpl_app_bookmark_manager_finalize (GObject *object)
{
  MplAppBookmarkManagerPrivate *priv = GET_PRIVATE (object);

  g_free (priv->path);

  G_OBJECT_CLASS (mpl_app_bookmark_manager_parent_class)->finalize (object);
}

static void
mpl_app_bookmark_manager_class_init (MplAppBookmarkManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MplAppBookmarkManagerPrivate));

  object_class->dispose = mpl_app_bookmark_manager_dispose;
  object_class->finalize = mpl_app_bookmark_manager_finalize;

  /**
   * MplAppBookmarkManager::bookmarks-changed:
   * @manager: manager that received the signal
   *
   * The ::bookmarks-changed signal is emitted when user changes their
   * bookmarked applications.
   */
  signals[BOOKMARKS_CHANGED] =
    g_signal_new ("bookmarks-changed",
                  MPL_TYPE_APP_BOOKMARK_MANAGER,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MplAppBookmarkManagerClass,
                                   bookmarks_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
_setup_file_monitor (MplAppBookmarkManager *manager,
                     gchar                   *uri)
{
  MplAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);
  GFile *file;
  GFileMonitor *monitor;
  GError *error = NULL;

  file = g_file_new_for_uri (uri);
  monitor = g_file_monitor_file (file,
                                 G_FILE_MONITOR_NONE,
                                 NULL,
                                 &error);
  if (error)
  {
    g_warning (G_STRLOC ": Error opening file monitor: %s",
               error->message);
    g_clear_error (&error);
  } else {
    g_signal_connect (monitor,
                      "changed",
                      (GCallback) _bookmark_desktop_file_changed_cb,
                      manager);
    g_hash_table_insert (priv->monitors_hash, uri, monitor);
  }

  g_object_unref (file);
}
static void
mpl_app_bookmark_manager_load (MplAppBookmarkManager *manager)
{
  MplAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);
  GList *removed_bookmarks;
  GError *error = NULL;
  gchar **uris;
  gchar *contents;
  gint i = 0;
  gchar *uri = NULL;

  if (!g_file_get_contents (priv->path,
                            &contents,
                            NULL,
                            &error))
  {
    g_critical (G_STRLOC ": Unable to open bookmarks file: %s",
                error->message);
    g_clear_error (&error);
    return;
  }

  /* We switched from ' ' delimiters to '\n' for meego 1.0
   * but still want to support the old file format. */
  if (strchr (contents, ' '))
    uris = g_strsplit (contents, " ", -1);
  else
    uris = g_strsplit (contents, "\n", -1);

  /* Any leftover bookmarks queued for removal? */
  removed_bookmarks = _list_pending_removals (manager, TRUE);

  for (i = 0; uris[i] != NULL; i++)
  {
    /* g_strsplit doesn't handle trailing '\n' */
    if (uris[i] == NULL || uris[i][0] == '\0')
      continue;

    /* The list takes ownership. */
    uri = g_strdup (uris[i]);

    if (removed_bookmarks == NULL ||
        !g_list_find_custom (removed_bookmarks, uri, (GCompareFunc) g_strcmp0))
    {
      priv->uris = g_list_append (priv->uris, uri);
      _setup_file_monitor (manager, uri);
    }
  }

  if (removed_bookmarks)
  {
    g_list_foreach (removed_bookmarks, (GFunc) g_free, NULL);
    g_list_free (removed_bookmarks);
  }

  g_strfreev (uris);
}

/**
 * mpl_app_bookmark_manager_save:
 * @manager: #MplAppBookmarkManager
 *
 * Saves current bookmarks to disk.
 */
void
mpl_app_bookmark_manager_save (MplAppBookmarkManager *manager)
{
  MplAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);
  gchar *contents;
  gchar **uris;
  gint i = 0;
  GList *l;
  GError *error = NULL;

  uris = g_new0 (gchar *, g_list_length (priv->uris) + 1);

  for (l = priv->uris; l; l = l->next)
  {
    uris[i] = (gchar *)l->data;
    i++;
  }

  uris[i] = NULL;

  contents = g_strjoinv ("\n", uris);

  if (!g_file_set_contents (priv->path,
                            contents,
                            -1,
                            &error))
  {
    g_critical (G_STRLOC ": Unable to save to bookmarks file: %s, retrying ...",
                error->message);
    g_clear_error (&error);

    /* Retry */
    mpl_app_bookmark_manager_idle_save (manager);
  }

  g_free (contents);
  g_free (uris);
}

static gboolean
_save_idle_cb (gpointer userdata)
{
  MplAppBookmarkManager *manager = (MplAppBookmarkManager *)userdata;
  MplAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);

  mpl_app_bookmark_manager_save (manager);
  priv->save_idle_id = 0;

  return FALSE;
}

static void
mpl_app_bookmark_manager_idle_save (MplAppBookmarkManager *manager)
{
  MplAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);

  if (priv->save_idle_id == 0)
  {
    priv->save_idle_id = g_idle_add (_save_idle_cb, manager);
  }
}

static void
_file_monitor_changed_cb (GFileMonitor      *monitor,
                          GFile             *file,
                          GFile             *other_file,
                          GFileMonitorEvent event,
                          gpointer          userdata)
{
  MplAppBookmarkManager *self = (MplAppBookmarkManager *)userdata;
  MplAppBookmarkManagerPrivate *priv = GET_PRIVATE (self);

  g_list_free (priv->uris);
  priv->uris = NULL;

  mpl_app_bookmark_manager_load (self);

  g_signal_emit (self, signals[BOOKMARKS_CHANGED], 0);
}

static void
mpl_app_bookmark_manager_init (MplAppBookmarkManager *self)
{
  MplAppBookmarkManagerPrivate *priv = GET_PRIVATE (self);
  GFile *f;
  GError *error = NULL;

  priv->path = g_build_filename (g_get_user_data_dir (),
                                 APP_BOOKMARK_FILENAME,
                                 NULL);

  /* Monitor the path, this will catch creation, deletion and change */
  f = g_file_new_for_path (priv->path);
  priv->monitor = g_file_monitor_file (f,
                                       G_FILE_MONITOR_NONE,
                                       NULL,
                                       &error);

  if (!priv->monitor)
  {
    g_warning (G_STRLOC ": Error opening file monitor: %s",
               error->message);
    g_clear_error (&error);
  } else {
    g_signal_connect (priv->monitor,
                      "changed",
                      (GCallback)_file_monitor_changed_cb,
                      self);
  }

  /* Monitors for the desktop files.
   * Keys are owned by "priv->uris". */
  priv->monitors_hash = g_hash_table_new_full (g_direct_hash,
                                               g_str_equal,
                                               NULL,
                                               g_object_unref);

  mpl_app_bookmark_manager_load (self);
}

/**
 * mpl_app_bookmark_manager_get_default:
 *
 * Returns an instance of #MplAppBookmarkManager object.
 *
 * Return value: #MplAppBookmarkManager; the caller holds a reference to the
 * object, and should release it using g_object_unref() when no longer needed.
 */
MplAppBookmarkManager *
mpl_app_bookmark_manager_get_default (void)
{
  static MplAppBookmarkManager *manager = NULL;

  if (!manager)
    {
      manager = g_object_new (MPL_TYPE_APP_BOOKMARK_MANAGER,
                              NULL);
      g_object_add_weak_pointer ((GObject *)manager, (gpointer)&manager);
    }
  else
    {
      g_object_ref (manager);
    }

  return manager;
}

/**
 * mpl_app_bookmark_manager_get_bookmarks:
 * @manager: #MplAppBookmarkManager
 *
 * Obtains list of current bookmarks.
 *
 * Return value: current bookmarks as a #GList of constant strings; the list is
 * owned by the caller and must be freed with g_list_free() when no longer
 * needed, however, the list data is owned by the manager and must not be freed.
 */
GList *
mpl_app_bookmark_manager_get_bookmarks (MplAppBookmarkManager *manager)
{
  MplAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);

  return g_list_copy (priv->uris);
}

/**
 * mpl_app_bookmark_manager_remove_uri:
 * @manager: #MplAppBookmarkManager
 * @uri: the bookmark to remove
 *
 * Removes bookmark identified by uri from the manager, and schedules the new
 * bookmark list to be saved when idle.
 */
void
mpl_app_bookmark_manager_remove_uri (MplAppBookmarkManager *manager,
                                     const gchar           *uri)
{
  MplAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);
  GList *l;

  g_return_if_fail (MPL_IS_APP_BOOKMARK_MANAGER (manager));

  for (l = priv->uris; l; l = l->next)
  {
    if (g_str_equal ((gchar *)l->data, uri))
    {
      g_free ((gchar *)l->data);
      priv->uris = g_list_delete_link (priv->uris, l);
    }
  }

  /* Remove monitor. */
  g_hash_table_remove (priv->monitors_hash, uri);

  mpl_app_bookmark_manager_idle_save (manager);
}

/**
 * mpl_app_bookmark_manager_add_uri:
 * @manager: #MplAppBookmarkManager
 * @uri: the bookmark to add
 *
 * Adds bookmark identified by uri to the manager, and schedules the new
 * bookmark list to be save in the next idle cycle.
 */
void
mpl_app_bookmark_manager_add_uri (MplAppBookmarkManager *manager,
                                  const gchar           *uri_in)
{
  MplAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);
  gchar *uri = NULL;

  g_return_if_fail (MPL_IS_APP_BOOKMARK_MANAGER (manager));

  /* The list takes ownership. */
  uri = g_strdup (uri_in);
  priv->uris = g_list_append (priv->uris, uri);

  _setup_file_monitor (manager, uri);

  mpl_app_bookmark_manager_idle_save (manager);
}
