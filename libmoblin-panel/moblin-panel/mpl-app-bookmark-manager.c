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


#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "mpl-app-bookmark-manager.h"

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

enum
{
  BOOKMARK_ADDED_SIGNAL,
  BOOKMARK_REMOVED_SIGNAL,
  LAST_SIGNAL
};

guint signals[LAST_SIGNAL] = { 0 };

static void
_bookmark_desktop_file_changed_cb (GFileMonitor      *monitor,
                                   GFile             *file,
                                   GFile             *other_file,
                                   GFileMonitorEvent event,
                                   gpointer          userdata)
{
  MplAppBookmarkManager *self = (MplAppBookmarkManager *)userdata;
  gchar *uri;

  /* Only care for removed apps that are bookmarked. */
  if (event != G_FILE_MONITOR_EVENT_DELETED)
    return;

  uri = g_file_get_uri (file);
  mpl_app_bookmark_manager_remove_uri (self, uri);
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

  signals[BOOKMARK_ADDED_SIGNAL] = 
    g_signal_new ("bookmark-added",
                  MPL_TYPE_APP_BOOKMARK_MANAGER,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MplAppBookmarkManagerClass, bookmark_added),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_STRING);

  signals[BOOKMARK_REMOVED_SIGNAL] =
    g_signal_new ("bookmark-removed",
                  MPL_TYPE_APP_BOOKMARK_MANAGER,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (MplAppBookmarkManagerClass, bookmark_removed),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_STRING);
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

  uris = g_strsplit (contents, " ", -1);

  for (i = 0; uris[i] != NULL; i++)
  {
    /* The list takes ownership. */
    uri = g_strdup (uris[i]);
    priv->uris = g_list_append (priv->uris, uri);

    _setup_file_monitor (manager, uri);
  }

  g_strfreev (uris);
}

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

  contents = g_strjoinv (" ", uris);

  if (!g_file_set_contents (priv->path, 
                            contents,
                            -1,
                            &error))
  {
    g_critical (G_STRLOC ": Unable to save to bookmarks file: %s",
                error->message);
    g_clear_error (&error);
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
  GList *l;
  gchar *uri;

  for (l = priv->uris; l; l=l->next)
  {
    uri = (gchar *)l->data;
    priv->uris = g_list_delete_link (priv->uris, l);
    g_signal_emit_by_name (self, "bookmark-removed", uri);
    g_free (uri);
  }
  priv->uris = NULL;

  mpl_app_bookmark_manager_load (self);

  for (l = priv->uris; l; l = l->next)
  {
    g_signal_emit_by_name (self, "bookmark-added", l->data);
  }
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

  return manager;
}

GList *
mpl_app_bookmark_manager_get_bookmarks (MplAppBookmarkManager *manager)
{
  MplAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);

  return g_list_copy (priv->uris);
}

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
  g_signal_emit_by_name (manager, "bookmark-removed", uri);
}

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
  g_signal_emit_by_name (manager, "bookmark-added", uri);
}

