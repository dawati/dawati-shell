#include "anerley-simple-grid-view.h"
#include <anerley/anerley-tile.h>

G_DEFINE_TYPE (AnerleySimpleGridView, anerley_simple_grid_view, NBTK_TYPE_GRID)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_SIMPLE_GRID_VIEW, AnerleySimpleGridViewPrivate))

typedef struct _AnerleySimpleGridViewPrivate AnerleySimpleGridViewPrivate;

struct _AnerleySimpleGridViewPrivate
{
  AnerleyFeed *feed;

  GHashTable *items_to_tiles;
};

enum
{
  PROP_0,
  PROP_FEED
};

static void anerley_simple_grid_view_set_feed (AnerleySimpleGridView *grid,
                                               AnerleyFeed           *feed);

static void
anerley_simple_grid_view_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  AnerleySimpleGridViewPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
  {
    case PROP_FEED:
      g_value_set_object (value, priv->feed);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_simple_grid_view_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value, 
                                       GParamSpec   *pspec)
{
  AnerleySimpleGridView *grid = (AnerleySimpleGridView *)object;

  switch (property_id)
  {
    case PROP_FEED:
      anerley_simple_grid_view_set_feed (grid,
                                         g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_simple_grid_view_dispose (GObject *object)
{
  AnerleySimpleGridView *grid = (AnerleySimpleGridView *)object;
  AnerleySimpleGridViewPrivate *priv = GET_PRIVATE (object);

  anerley_simple_grid_view_set_feed (grid, NULL);

  if (priv->items_to_tiles)
  {
    g_hash_table_unref (priv->items_to_tiles);
    priv->items_to_tiles = NULL;
  }

  G_OBJECT_CLASS (anerley_simple_grid_view_parent_class)->dispose (object);
}

static void
anerley_simple_grid_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (anerley_simple_grid_view_parent_class)->finalize (object);
}

static void
anerley_simple_grid_view_class_init (AnerleySimpleGridViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleySimpleGridViewPrivate));

  object_class->get_property = anerley_simple_grid_view_get_property;
  object_class->set_property = anerley_simple_grid_view_set_property;
  object_class->dispose = anerley_simple_grid_view_dispose;
  object_class->finalize = anerley_simple_grid_view_finalize;

  pspec = g_param_spec_object ("feed",
                               "The feed",
                               "Feed to show",
                               ANERLEY_TYPE_FEED,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_FEED, pspec);
}

static void
anerley_simple_grid_view_init (AnerleySimpleGridView *self)
{
  AnerleySimpleGridViewPrivate *priv = GET_PRIVATE (self);

  priv->items_to_tiles = g_hash_table_new_full (g_direct_hash,
                                                g_direct_equal,
                                                NULL,
                                                NULL);
}

NbtkWidget *
anerley_simple_grid_view_new (AnerleyFeed *feed)
{
  return g_object_new (ANERLEY_TYPE_SIMPLE_GRID_VIEW, 
                       "feed",
                       feed,
                       NULL);
}

static void
_feed_items_added_cb (AnerleyFeed *feed,
                      GList       *items,
                      gpointer     userdata)
{
  AnerleySimpleGridView *grid = (AnerleySimpleGridView *)userdata;
  GList *l;
  AnerleyItem *item;
  NbtkWidget *tile;

  for (l = items; l; l = l->next)
  {
    item = (AnerleyItem *)l->data;
    tile = anerley_tile_new (item);
    clutter_actor_set_size ((ClutterActor *)tile, 220, 90);
    clutter_container_add_actor (CLUTTER_CONTAINER (grid),
                                 (ClutterActor *)tile);
  }
}

static void
_feed_items_changed_cb (AnerleyFeed *feed,
                        GList       *items,
                        gpointer     userdata)
{

}

static void
_feed_items_removed_cb (AnerleyFeed *feed,
                        GList       *items,
                        gpointer     userdata)
{
  AnerleySimpleGridView *grid = (AnerleySimpleGridView *)userdata;
  AnerleySimpleGridViewPrivate *priv = GET_PRIVATE (grid);
  GList *l;
  AnerleyItem *item;
  NbtkWidget *tile;

  for (l = items; l; l = l->next)
  {
    item = (AnerleyItem *)l->data;
    tile = g_hash_table_lookup (priv->items_to_tiles,
                                item);
    if (tile)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (grid),
                                      (ClutterActor *)tile);
    }
  }
}

static void
anerley_simple_grid_view_set_feed (AnerleySimpleGridView *view,
                                   AnerleyFeed           *feed)
{
  AnerleySimpleGridViewPrivate *priv = GET_PRIVATE (view);

  if (priv->feed)
  {
    g_signal_handlers_disconnect_by_func (priv->feed,
                                          _feed_items_added_cb,
                                          view);
    g_signal_handlers_disconnect_by_func (priv->feed,
                                          _feed_items_changed_cb,
                                          view);
    g_signal_handlers_disconnect_by_func (priv->feed,
                                          _feed_items_removed_cb,
                                          view);

    g_object_unref (priv->feed);
    priv->feed = NULL;
  }

  if (feed)
  {
    priv->feed = g_object_ref (feed);

    g_signal_connect (feed,
                      "items-added",
                      (GCallback)_feed_items_added_cb,
                      view);
    g_signal_connect (feed,
                      "items-changed",
                      (GCallback)_feed_items_changed_cb,
                      view);
    g_signal_connect (feed,
                      "items-removed",
                      (GCallback)_feed_items_removed_cb,
                      view);
  }
}
