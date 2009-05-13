#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "penge-app-bookmark-manager.h"

G_DEFINE_TYPE (PengeAppBookmarkManager, penge_app_bookmark_manager, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_APP_BOOKMARK_MANAGER, PengeAppBookmarkManagerPrivate))

typedef struct _PengeAppBookmarkManagerPrivate PengeAppBookmarkManagerPrivate;

struct _PengeAppBookmarkManagerPrivate {
    GBookmarkFile *bookmarks;
    gchar *path;
    GFileMonitor *monitor;
    GHashTable *uris_to_bookmarks;
    guint save_idle_id;
};

#define APP_BOOKMARK_FILENAME "favourite-apps.xbel"
#define PENGE_APP_NAME "penge"

enum
{
  BOOKMARK_ADDED_SIGNAL,
  BOOKMARK_REMOVED_SIGNAL,
  LAST_SIGNAL
};

guint signals[LAST_SIGNAL] = { 0 };

static void
penge_app_bookmark_manager_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_app_bookmark_manager_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_app_bookmark_manager_dispose (GObject *object)
{
  PengeAppBookmarkManagerPrivate *priv = GET_PRIVATE (object);

  if (priv->bookmarks)
  {
    if (priv->save_idle_id > 0)
    {
      g_source_remove (priv->save_idle_id);
      penge_app_bookmark_manager_save ((PengeAppBookmarkManager *)object);
    }

    g_bookmark_file_free (priv->bookmarks);
    priv->bookmarks = NULL;
  }

  if (priv->monitor)
  {
    g_file_monitor_cancel (priv->monitor);
    g_object_unref (priv->monitor);
    priv->monitor = NULL;
  }

  if (priv->uris_to_bookmarks)
  {
    g_hash_table_unref (priv->uris_to_bookmarks);
    priv->uris_to_bookmarks = NULL;
  }

  G_OBJECT_CLASS (penge_app_bookmark_manager_parent_class)->dispose (object);
}

static void
penge_app_bookmark_manager_finalize (GObject *object)
{
  PengeAppBookmarkManagerPrivate *priv = GET_PRIVATE (object);

  g_free (priv->path);

  G_OBJECT_CLASS (penge_app_bookmark_manager_parent_class)->finalize (object);
}

static void
penge_app_bookmark_manager_class_init (PengeAppBookmarkManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeAppBookmarkManagerPrivate));

  object_class->get_property = penge_app_bookmark_manager_get_property;
  object_class->set_property = penge_app_bookmark_manager_set_property;
  object_class->dispose = penge_app_bookmark_manager_dispose;
  object_class->finalize = penge_app_bookmark_manager_finalize;

  signals[BOOKMARK_ADDED_SIGNAL] = 
    g_signal_new ("bookmark-added",
                  PENGE_TYPE_APP_BOOKMARK_MANAGER,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (PengeAppBookmarkManagerClass, bookmark_added),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE,
                  1,
                  PENGE_TYPE_APP_BOOKMARK);

  signals[BOOKMARK_REMOVED_SIGNAL] =
    g_signal_new ("bookmark-removed",
                  PENGE_TYPE_APP_BOOKMARK_MANAGER,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (PengeAppBookmarkManagerClass, bookmark_removed),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_STRING);
}

void
penge_app_bookmark_manager_load (PengeAppBookmarkManager *manager)
{
  PengeAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);
  GError *error = NULL;
  gchar **uris, *uri;
  gint i = 0;
  PengeAppBookmark *bookmark;

  if (!g_bookmark_file_load_from_file (priv->bookmarks,
                                       priv->path,
                                       &error))
  {
    g_warning (G_STRLOC ": Error loading bookmark file: %s",
               error->message);
    g_clear_error (&error);
  }

  uris = g_bookmark_file_get_uris (priv->bookmarks, NULL);

  for (uri = uris[i]; uri; uri = uris[i])
  {
    bookmark = g_hash_table_lookup (priv->uris_to_bookmarks,
                                    uri);

    if (bookmark)
    {
      i++;
      continue;
    }

    bookmark = penge_app_bookmark_new ();

    g_free (bookmark->uri);
    bookmark->uri = g_strdup (uri);;

    g_free (bookmark->application_name);
    bookmark->application_name = g_bookmark_file_get_title (priv->bookmarks,
                                                            uri,
                                                            &error);
    if (error)
    {
      g_warning (G_STRLOC ": Error when retrieving title: %s",
                 error->message);
      g_clear_error (&error);
    }

    g_free (bookmark->app_exec);
    if (!g_bookmark_file_get_app_info (priv->bookmarks,
                                      uri,
                                      PENGE_APP_NAME,
                                      &(bookmark->app_exec),
                                      NULL,
                                      NULL,
                                      &error))
    {
      g_warning (G_STRLOC ": Error when retrieving exec string: %s",
                 error->message);
      g_clear_error (&error);
    }

    g_hash_table_insert (priv->uris_to_bookmarks,
                         g_strdup (uri),
                         penge_app_bookmark_ref (bookmark));

    penge_app_bookmark_unref (bookmark);
    i++;
  }

  g_strfreev (uris);
}

void
penge_app_bookmark_manager_save (PengeAppBookmarkManager *manager)
{
  PengeAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);
  GError *error = NULL;

  if (!g_bookmark_file_to_file (priv->bookmarks,
                                priv->path,
                                &error))
  {
    g_warning (G_STRLOC ": Error when writing bookmarks: %s",
               error->message);
    g_clear_error (&error);
  }
}

static gboolean
_save_idle_cb (gpointer userdata)
{
  PengeAppBookmarkManager *manager = (PengeAppBookmarkManager *)userdata;
  PengeAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);

  penge_app_bookmark_manager_save (manager);
  priv->save_idle_id = 0;

  return FALSE;
}

static void
penge_app_bookmark_manager_idle_save (PengeAppBookmarkManager *manager)
{
  PengeAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);

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
  /* not doing anything here yet */
}

static void
penge_app_bookmark_manager_init (PengeAppBookmarkManager *self)
{
  PengeAppBookmarkManagerPrivate *priv = GET_PRIVATE (self);
  GFile *f;
  GError *error = NULL;

  priv->uris_to_bookmarks = g_hash_table_new_full (g_str_hash,
                                                   g_str_equal,
                                                   g_free,
                                                   (GDestroyNotify)penge_app_bookmark_unref);
  priv->bookmarks = g_bookmark_file_new ();
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
}


PengeAppBookmarkManager *
penge_app_bookmark_manager_get_default (void)
{
  static PengeAppBookmarkManager *manager = NULL;

  if (!manager)
  {
    manager = g_object_new (PENGE_TYPE_APP_BOOKMARK_MANAGER,
                            NULL);
    g_object_add_weak_pointer ((GObject *)manager, (gpointer)&manager);
  }

  return manager;
}

GList *
penge_app_bookmark_manager_get_bookmarks (PengeAppBookmarkManager *manager)
{
  PengeAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);

  return g_hash_table_get_values (priv->uris_to_bookmarks);
}

gboolean
penge_app_bookmark_manager_remove_by_uri (PengeAppBookmarkManager *manager,
                                          const gchar             *uri,
                                          GError                 **error_out)
{
  PengeAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);
  GError *error = NULL;

  if (!g_hash_table_remove (priv->uris_to_bookmarks, uri))
  {
    g_warning (G_STRLOC ": No such uri known.");

    /* TODO: Set error_out to a GError. */
    return FALSE;
  }

  if (!g_bookmark_file_remove_item (priv->bookmarks, uri, &error))
  {
    g_warning (G_STRLOC ": Error removing item: %s", error->message);
    g_propagate_error (error_out, error);
    return FALSE;
  }

  penge_app_bookmark_manager_idle_save (manager);
  g_signal_emit (manager, signals[BOOKMARK_REMOVED_SIGNAL], 0, uri);

  return TRUE;
}

gboolean
penge_app_bookmark_manager_add_from_uri (PengeAppBookmarkManager *manager,
                                         const gchar             *uri,
                                         GError                 **error_out)
{
  PengeAppBookmarkManagerPrivate *priv = GET_PRIVATE (manager);
  gchar *path;
  GAppInfo *app_info;
  GError *error = NULL;
  PengeAppBookmark *bookmark;

  /* Check if we already have this URI present */
  bookmark = g_hash_table_lookup (priv->uris_to_bookmarks,
                                  uri);

  if (bookmark)
  {
    return TRUE;
  }

  path = g_filename_from_uri (uri, NULL, &error);

  if (!path)
  {
    g_warning (G_STRLOC ": Error converting uri to path: %s",
               error->message);
    g_propagate_error (error_out, error);
    goto error;
  }

  app_info = G_APP_INFO (g_desktop_app_info_new_from_filename (path));
  if (!app_info)
  {
    /* TODO: Set error_out to a correct GError. */
    *error_out = g_error_new (0, 0, G_STRLOC ": Error loading desktop file: %s",
                              path);
    goto error;
  }

  bookmark = penge_app_bookmark_new ();
  bookmark->uri = g_strdup (uri);
  bookmark->application_name = g_strdup (g_app_info_get_name (app_info));
  bookmark->app_exec = g_strdup (g_app_info_get_executable (app_info));

  g_free (path);
  g_object_unref (app_info);

  /* Create the entry in the GBookmarkFile object */
  g_bookmark_file_set_title (priv->bookmarks, uri, bookmark->application_name);
  if (!g_bookmark_file_set_app_info (priv->bookmarks,
                                     uri,
                                     PENGE_APP_NAME,
                                     bookmark->app_exec,
                                     1,
                                     time (NULL),
                                     &error))
  {
    g_warning (G_STRLOC ": Error setting application execute information: %s",
               error->message);
    g_propagate_error (error_out, error);
    g_bookmark_file_remove_item (priv->bookmarks, uri, NULL);
    goto error;
  }

  g_hash_table_insert (priv->uris_to_bookmarks,
                       g_strdup (uri),
                       bookmark);

  penge_app_bookmark_manager_idle_save (manager);
  g_signal_emit (manager, signals[BOOKMARK_ADDED_SIGNAL], 0, bookmark);

  return TRUE;

error:
  if (bookmark)
    penge_app_bookmark_free (bookmark);
  return FALSE;
}

PengeAppBookmark *
penge_app_bookmark_ref (PengeAppBookmark *bookmark)
{
  g_atomic_int_inc (&(bookmark->ref_cnt));
  return bookmark;
}

void
penge_app_bookmark_unref (PengeAppBookmark *bookmark)
{
  if (g_atomic_int_dec_and_test (&(bookmark->ref_cnt)))
  {
    penge_app_bookmark_free (bookmark);
  }
}

PengeAppBookmark *
penge_app_bookmark_new (void)
{
  return penge_app_bookmark_ref (g_slice_new0 (PengeAppBookmark));
}

void
penge_app_bookmark_free (PengeAppBookmark *bookmark)
{
  g_free (bookmark->application_name);
  g_free (bookmark->app_exec);
}

GType
penge_app_bookmark_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    type = g_boxed_type_register_static ("PengeAppBookmark",
                                         (GBoxedCopyFunc)penge_app_bookmark_ref,
                                         (GBoxedFreeFunc)penge_app_bookmark_unref);
  }

  return type;
}
