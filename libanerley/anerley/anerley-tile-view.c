#include "anerley-tile-view.h"
#include <anerley/anerley-tile.h>

typedef struct {
  NbtkCellRenderer parent;
} AnerleyTileRenderer;

typedef struct {
  NbtkCellRendererClass parent_class;
} AnerleyTileRendererClass;

GType anerley_tile_renderer_get_type (void);

#define ANERLEY_TILE_TYPE_RENDERER anerley_tile_renderer_get_type()

G_DEFINE_TYPE (AnerleyTileRenderer, anerley_tile_renderer, NBTK_TYPE_CELL_RENDERER)

ClutterActor *
anerley_tile_renderer_get_actor (NbtkCellRenderer *renderer)
{
  ClutterActor *tile;

  tile = g_object_new (ANERLEY_TYPE_TILE,
                       NULL);
  clutter_actor_set_size ((ClutterActor *)tile, 200, 90);

  return tile;
}

static void
anerley_tile_renderer_class_init (AnerleyTileRendererClass *klass)
{
  NbtkCellRendererClass *renderer = NBTK_CELL_RENDERER_CLASS (klass);

  renderer->get_actor = anerley_tile_renderer_get_actor;
}

static void
anerley_tile_renderer_init (AnerleyTileRenderer *renderer)
{
}


G_DEFINE_TYPE (AnerleyTileView, anerley_tile_view, NBTK_TYPE_ICON_VIEW)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_TILE_VIEW, AnerleyTileViewPrivate))

typedef struct _AnerleyTileViewPrivate AnerleyTileViewPrivate;

struct _AnerleyTileViewPrivate {
  gpointer nuffink;
};

enum
{
  PROP_0,
  PROP_MODEL
};

enum
{
  ITEM_ACTIVATED,
  LAST_SIGNAL
};

guint signals[LAST_SIGNAL] = { 0 };

static void
anerley_tile_view_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tile_view_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
    case PROP_MODEL:
      nbtk_icon_view_set_model (NBTK_ICON_VIEW (object),
                                g_value_get_object (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tile_view_dispose (GObject *object)
{
  G_OBJECT_CLASS (anerley_tile_view_parent_class)->dispose (object);
}

static void
anerley_tile_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (anerley_tile_view_parent_class)->finalize (object);
}

static void
anerley_tile_view_class_init (AnerleyTileViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyTileViewPrivate));

  object_class->get_property = anerley_tile_view_get_property;
  object_class->set_property = anerley_tile_view_set_property;
  object_class->dispose = anerley_tile_view_dispose;
  object_class->finalize = anerley_tile_view_finalize;

  pspec = g_param_spec_object ("model",
                               "The anerley feed model",
                               "Model of feed to render",
                               ANERLEY_TYPE_FEED_MODEL,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   pspec);

  signals[ITEM_ACTIVATED] =
    g_signal_new ("item-activated",
                  ANERLEY_TYPE_TILE_VIEW,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnerleyTileViewClass, item_activated),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  ANERLEY_TYPE_ITEM);

}

static void
anerley_tile_view_init (AnerleyTileView *self)
{
  nbtk_icon_view_set_cell_renderer (NBTK_ICON_VIEW (self),
                                    g_object_new (ANERLEY_TILE_TYPE_RENDERER,
                                                  NULL));
  nbtk_icon_view_add_attribute (NBTK_ICON_VIEW (self), "item", 0);
}

NbtkWidget *
anerley_tile_view_new (AnerleyFeedModel *model)
{
  return g_object_new (ANERLEY_TYPE_TILE_VIEW,
                       "model",
                       model,
                       NULL);
}


