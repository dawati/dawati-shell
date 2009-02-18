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

static void penge_apps_pane_populate (PengeAppsPane *pane);

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
penge_apps_pane_init (PengeAppsPane *self)
{
  PengeAppsPanePrivate *priv = GET_PRIVATE (self);

  priv->manager = penge_app_bookmark_manager_get_default ();
  penge_app_bookmark_manager_load (priv->manager);

  priv->uris_to_actors = g_hash_table_new_full (g_str_hash,
                                                g_str_equal,
                                                g_free,
                                                NULL);
  penge_apps_pane_populate (self);
}

static void
penge_apps_pane_populate (PengeAppsPane *pane)
{
  PengeAppsPanePrivate *priv = GET_PRIVATE (pane);
  GList *bookmarks, *l;
  ClutterActor *actor;
  gint count = 0;
  PengeAppBookmark *bookmark;

  bookmarks = penge_app_bookmark_manager_get_bookmarks (priv->manager);

  for (l = bookmarks; l; l = l->next)
  {
    bookmark = (PengeAppBookmark *)l->data;
    actor = g_object_new (PENGE_TYPE_APP_TILE,
                          "bookmark",
                          bookmark,
                          NULL);
    nbtk_table_add_actor (NBTK_TABLE (pane),
                          actor,
                          count,
                          0);
    clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                 actor,
                                 "x-expand",
                                 FALSE,
                                 "y-expand",
                                 FALSE,
                                 NULL);
    count++;
  }

  g_list_free (bookmarks);
}

static void
_manager_bookmark_added_cb (PengeAppBookmarkManager *manager,
                            PengeAppBookmark        *bookmark,
                            gpointer                 userdata)
{

}

static void
_manager_bookmark_removed_cb (PengeAppBookmarkManager *manager,
                              const gchar             *uri,
                              gpointer                 userdata)
{

}
