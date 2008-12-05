#include "penge-tile.h"

#include <nbtk/nbtk.h>

G_DEFINE_TYPE (PengeTile, penge_tile, CLUTTER_TYPE_ACTOR)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_TILE, PengeTilePrivate))

typedef struct _PengeTilePrivate PengeTilePrivate;

struct _PengeTilePrivate {
    ClutterActor *frame;
    ClutterActor *child;
};

#define PADDING 0
#define BG_FILENAME "data/frame-bg.png"

#define FRAME_LEFT 7
#define FRAME_TOP 5
#define FRAME_RIGHT 64
#define FRAME_BOTTOM 58

#define MARGIN_LEFT 3
#define MARGIN_TOP 3
#define MARGIN_RIGHT 10
#define MARGIN_BOTTOM 8

enum {
  PROP0,
  PROP_CHILD,
};

static void
penge_tile_get_property (GObject    *object, 
                         guint       property_id,
                         GValue     *value, 
                         GParamSpec *pspec)
{
  PengeTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_CHILD:
      g_value_set_object (value, priv->child);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_tile_set_property (GObject      *object, 
                         guint         property_id,
                         const GValue *value, 
                         GParamSpec   *pspec)
{
  PengeTilePrivate *priv = GET_PRIVATE (object);
  
  switch (property_id) {
    case PROP_CHILD:
      if (priv->child)
      {
        clutter_actor_set_parent (priv->child, NULL);
        g_object_unref (priv->child);
      }

      priv->child = g_value_dup_object (value);
      clutter_actor_set_position (priv->child, PADDING, PADDING);
      clutter_actor_set_parent (priv->child, object);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_tile_dispose (GObject *object)
{
  PengeTilePrivate *priv = GET_PRIVATE (object);

  if (priv->child)
  {
    g_object_unref (priv->child);
    priv->child = NULL;
  }

  if (priv->frame)
  {
    g_object_unref (priv->frame);
    priv->frame = NULL;
  }

  G_OBJECT_CLASS (penge_tile_parent_class)->dispose (object);
}

static void
penge_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_tile_parent_class)->finalize (object);
}

static void
_tile_paint (ClutterActor *actor)
{
  PengeTilePrivate *priv = GET_PRIVATE (actor);

  clutter_actor_paint (priv->frame);

  if (priv->child)
    clutter_actor_paint (priv->child);
}

static void
_tile_allocate (ClutterActor          *actor,
                const ClutterActorBox *box,
                gboolean               origin_changed)
{
  PengeTilePrivate *priv = GET_PRIVATE (actor);
  gint w, h;
  ClutterUnit child_wu, child_hu;
  ClutterActorBox child_box;
  ClutterActorBox inner_box;

  /* chain up to set the allocation of the actor */
  CLUTTER_ACTOR_CLASS (penge_tile_parent_class)->allocate (actor, 
                                                           box, 
                                                           origin_changed);

  w = CLUTTER_UNITS_TO_DEVICE (box->x2 - box->x1);
  h = CLUTTER_UNITS_TO_DEVICE (box->y2 - box->y1);

  /* 
   * Represents the maximum inner space available to the child including
   * framing and padding
   */
  inner_box.x1 = CLUTTER_UNITS_FROM_DEVICE (MARGIN_LEFT + PADDING);
  inner_box.y1 = CLUTTER_UNITS_FROM_DEVICE (MARGIN_TOP + PADDING);
  inner_box.x2 = CLUTTER_UNITS_FROM_DEVICE (w - (MARGIN_RIGHT + PADDING));
  inner_box.y2 = CLUTTER_UNITS_FROM_DEVICE (h - (MARGIN_BOTTOM + PADDING));

  /* The frame should always match the size of the tile */
  clutter_actor_set_size (priv->frame, w, h);

  if (priv->child)
  {
    /* 
     * Get the child size and then it's preferred width and height and then
     * center it inside the inner box, which is thus relative to the tile
     */
    clutter_actor_get_preferred_width (priv->child, -1, NULL, &child_wu);
    clutter_actor_get_preferred_height (priv->child, -1, NULL, &child_hu);

    child_box.x1 = inner_box.x1 + (((inner_box.x2 - inner_box.x1) - child_wu) / 2);
    child_box.x2 = child_box.x1 + child_wu;

    child_box.y1 = inner_box.y1 + (((inner_box.y2 - inner_box.y1) - child_hu) / 2);
    child_box.y2 = child_box.y1 + child_hu;

    clutter_actor_allocate (priv->child, &child_box, origin_changed);
  }
}

static void
_tile_get_preferred_width (ClutterActor *actor,
                           ClutterUnit   for_height,
                           ClutterUnit  *min_width_p,
                           ClutterUnit  *nat_width_p)
{
  PengeTilePrivate *priv = GET_PRIVATE (actor);

  ClutterUnit min_child_width;
  ClutterUnit nat_child_width;

  if (priv->child)
  {
    clutter_actor_get_preferred_width (priv->child,
                                       for_height,
                                       &min_child_width,
                                       &nat_child_width);

    *min_width_p = min_child_width + 
                   CLUTTER_UNITS_FROM_DEVICE (MARGIN_LEFT) +
                   CLUTTER_UNITS_FROM_DEVICE (MARGIN_RIGHT) +
                   CLUTTER_UNITS_FROM_DEVICE (PADDING * 2);


    *nat_width_p = nat_child_width + 
                   CLUTTER_UNITS_FROM_DEVICE (MARGIN_LEFT) +
                   CLUTTER_UNITS_FROM_DEVICE (MARGIN_RIGHT) +
                   CLUTTER_UNITS_FROM_DEVICE (PADDING * 2);
  } else {
    /* Just use the size of the frame */
    clutter_actor_get_preferred_width (priv->frame,
                                       for_height,
                                       min_width_p,
                                       nat_width_p);
  }
}

static void
_tile_get_preferred_height (ClutterActor *actor,
                            ClutterUnit   for_width,
                            ClutterUnit  *min_height_p,
                            ClutterUnit  *nat_height_p)
{
  PengeTilePrivate *priv = GET_PRIVATE (actor);

  ClutterUnit min_child_height;
  ClutterUnit nat_child_height;

  if (priv->child)
  {
    clutter_actor_get_preferred_height (priv->child,
                                       for_width,
                                       &min_child_height,
                                       &nat_child_height);

    *min_height_p = min_child_height + 
                   CLUTTER_UNITS_FROM_DEVICE (MARGIN_TOP) +
                   CLUTTER_UNITS_FROM_DEVICE (MARGIN_BOTTOM) +
                   CLUTTER_UNITS_FROM_DEVICE (PADDING * 2);


    *nat_height_p = nat_child_height + 
                   CLUTTER_UNITS_FROM_DEVICE (MARGIN_TOP) +
                   CLUTTER_UNITS_FROM_DEVICE (MARGIN_BOTTOM) +
                   CLUTTER_UNITS_FROM_DEVICE (PADDING * 2);
  } else {
    /* Just use the size of the frame */
    clutter_actor_get_preferred_height (priv->frame,
                                       for_width,
                                       min_height_p,
                                       nat_height_p);
  }
}

static void
penge_tile_class_init (PengeTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeTilePrivate));

  object_class->get_property = penge_tile_get_property;
  object_class->set_property = penge_tile_set_property;
  object_class->dispose = penge_tile_dispose;
  object_class->finalize = penge_tile_finalize;

  actor_class->allocate = _tile_allocate;
  actor_class->paint = _tile_paint;
  actor_class->get_preferred_width = _tile_get_preferred_width;
  actor_class->get_preferred_height = _tile_get_preferred_height;

  pspec = g_param_spec_object ("child",
                               "child",
                               "Child of tile",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_CHILD, pspec);
}

static void
penge_tile_init (PengeTile *self)
{
  PengeTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor *bg_tex;
  GError *error = NULL;

  bg_tex = clutter_texture_new_from_file (BG_FILENAME,
                                          &error);

  if (bg_tex)
  {
    priv->frame = nbtk_texture_frame_new ((ClutterTexture *)bg_tex,
                                          FRAME_LEFT,
                                          FRAME_TOP,
                                          FRAME_RIGHT,
                                          FRAME_BOTTOM);
    clutter_actor_set_parent (priv->frame, (ClutterActor *)self);
  } else {
    g_warning (G_STRLOC ": Unable to find texture for frame: %s", 
               error->message);
    g_clear_error (&error);
  }
}

ClutterActor *
penge_tile_new (void)
{
  return g_object_new (PENGE_TYPE_TILE, NULL);
}

