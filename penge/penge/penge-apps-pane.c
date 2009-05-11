#include <gtk/gtk.h>

#include "penge-apps-pane.h"
#include "penge-app-tile.h"
#include "penge-app-bookmark-manager.h"

G_DEFINE_TYPE (PengeAppsPane, penge_apps_pane, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_APPS_PANE, PengeAppsPanePrivate))

typedef struct _PengeAppsPanePrivate PengeAppsPanePrivate;

struct _PengeAppsPanePrivate {
  PengeAppBookmarkManager *manager;

  GHashTable *uris_to_actors;
};

#define ROW_SIZE 4
#define MAX_COUNT 8

static void penge_apps_pane_update (PengeAppsPane *pane);

static void
penge_apps_pane_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_apps_pane_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_apps_pane_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_apps_pane_parent_class)->dispose (object);
}

static void
penge_apps_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_apps_pane_parent_class)->finalize (object);
}

static void
penge_apps_pane_class_init (PengeAppsPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeAppsPanePrivate));

  object_class->get_property = penge_apps_pane_get_property;
  object_class->set_property = penge_apps_pane_set_property;
  object_class->dispose = penge_apps_pane_dispose;
  object_class->finalize = penge_apps_pane_finalize;
}

static void
_manager_bookmark_added_cb (PengeAppBookmarkManager *manager,
                            PengeAppBookmark        *bookmark,
                            gpointer                 userdata)
{
  penge_apps_pane_update ((PengeAppsPane *)userdata);
}

static void
_manager_bookmark_removed_cb (PengeAppBookmarkManager *manager,
                              const gchar             *uri,
                              gpointer                 userdata)
{
  penge_apps_pane_update ((PengeAppsPane *)userdata);
}

static void
penge_apps_pane_init (PengeAppsPane *self)
{
  PengeAppsPanePrivate *priv = GET_PRIVATE (self);

  priv->manager = penge_app_bookmark_manager_get_default ();
  penge_app_bookmark_manager_load (priv->manager);

  g_signal_connect (priv->manager,
                    "bookmark-added",
                    (GCallback)_manager_bookmark_added_cb,
                    self);
  g_signal_connect (priv->manager,
                    "bookmark-removed",
                    (GCallback)_manager_bookmark_removed_cb,
                    self);

  priv->uris_to_actors = g_hash_table_new_full (g_str_hash,
                                                g_str_equal,
                                                g_free,
                                                NULL);

  nbtk_table_set_row_spacing (NBTK_TABLE (self), 8);

  penge_apps_pane_update (self);
}

static void
penge_apps_pane_update (PengeAppsPane *pane)
{
  PengeAppsPanePrivate *priv = GET_PRIVATE (pane);
  GList *bookmarks, *l, *to_remove;
  ClutterActor *actor;
  gint count = 0;
  PengeAppBookmark *bookmark;
  gchar *path;
  GError *error = NULL;

  bookmarks = penge_app_bookmark_manager_get_bookmarks (priv->manager);

  to_remove = g_hash_table_get_keys (priv->uris_to_actors);

  for (l = bookmarks; l && count < MAX_COUNT; l = l->next)
  {
    bookmark = (PengeAppBookmark *)l->data;

    actor = g_hash_table_lookup (priv->uris_to_actors,
                                 bookmark->uri);

    /* Check if this URI is on the system */
    path = g_filename_from_uri (bookmark->uri, NULL, &error);

    if (error)
    {
      g_warning (G_STRLOC ": Error converting uri to path: %s",
                 error->message);
      g_clear_error (&error);

      if (actor)
      {
        clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                        actor);
      }

      continue;
    }

    if (!g_file_test (path, G_FILE_TEST_EXISTS))
    {
      /* skip those that have a missing .desktop file */
      g_free (path);
      continue;
    }

    g_free (path);

    if (actor)
    {
      clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                   actor,
                                   "row",
                                   count / ROW_SIZE,
                                   "col",
                                   count % ROW_SIZE,
                                   NULL);
    } else {
      actor = g_object_new (PENGE_TYPE_APP_TILE,
                            "bookmark",
                            bookmark,
                            NULL);
      nbtk_table_add_actor (NBTK_TABLE (pane),
                            actor,
                            count / ROW_SIZE,
                            count % ROW_SIZE);
      clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                   actor,
                                   "x-expand",
                                   FALSE,
                                   "y-expand",
                                   FALSE,
                                   NULL);
      g_hash_table_insert (priv->uris_to_actors,
                           g_strdup (bookmark->uri),
                           actor);
    }

    /* Found, so don't remove */
    /* This craziness is because the allocated string is probably different */
    to_remove = g_list_delete_link (to_remove,
                                    g_list_find_custom (to_remove,
                                                        bookmark->uri,
                                                        (GCompareFunc)g_strcmp0));
    count++;
  }

  g_list_free (bookmarks);

  for (l = to_remove; l; l = g_list_delete_link (l, l))
  {
    actor = g_hash_table_lookup (priv->uris_to_actors,
                                 (gchar *)(l->data));
    clutter_container_remove_actor (CLUTTER_CONTAINER (pane),
                                    actor);
    g_hash_table_remove (priv->uris_to_actors, (gchar *)(l->data));
  }
}

