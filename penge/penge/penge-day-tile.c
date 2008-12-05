#include "penge-day-tile.h"

#include <clutter/clutter.h>
#include <libjana/jana.h>

G_DEFINE_TYPE (PengeDayTile, penge_day_tile, PENGE_TYPE_TILE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_DAY_TILE, PengeDayTilePrivate))

typedef struct _PengeDayTilePrivate PengeDayTilePrivate;

struct _PengeDayTilePrivate {
    JanaTime *time;

    ClutterActor *day_label;
    ClutterActor *date_label;
};

enum
{
  PROP_0,
  PROP_TIME
};

static void
penge_day_tile_get_property (GObject    *object, 
                             guint       property_id,
                             GValue     *value, 
                             GParamSpec *pspec)
{
  PengeDayTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_TIME:
      g_value_set_object (value, time);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_day_tile_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  PengeDayTilePrivate *priv = GET_PRIVATE (object);
  gchar *day_str;
  gchar *date_str;

  switch (property_id) {
    case PROP_TIME:
      if (priv->time)
      {
        g_object_unref (priv->time);
        priv->time = NULL;
      }

      priv->time = g_value_dup_object (value);

      if (priv->time)
      {
        day_str = jana_utils_strftime (priv->time, "%A");
        clutter_label_set_text (CLUTTER_LABEL (priv->day_label),
                                day_str);
        g_free (day_str);

        date_str = jana_utils_strftime (priv->time, "%d");
        clutter_label_set_text (CLUTTER_LABEL (priv->date_label),
                                date_str);
        g_free (date_str);
      }

      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_day_tile_dispose (GObject *object)
{
  PengeDayTilePrivate *priv = GET_PRIVATE (object);

  if (priv->day_label)
  {
    g_object_unref (priv->day_label);
    priv->day_label = NULL;
  }

  if (priv->date_label)
  {
    g_object_unref (priv->date_label);
    priv->date_label = NULL;
  }

  G_OBJECT_CLASS (penge_day_tile_parent_class)->dispose (object);
}

static void
penge_day_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_day_tile_parent_class)->finalize (object);
}

static void
_day_tile_paint (ClutterActor *actor)
{
  PengeDayTilePrivate *priv = GET_PRIVATE (actor);

  CLUTTER_ACTOR_CLASS (penge_day_tile_parent_class)->paint (actor);

  clutter_actor_paint (priv->day_label);
  clutter_actor_paint (priv->date_label);
}

#define PADDING 5

static void
_day_tile_allocate (ClutterActor          *actor,
                    const ClutterActorBox *box,
                    gboolean               origin_changed)
{
  PengeDayTilePrivate *priv = GET_PRIVATE (actor);
  gint w, h;
  ClutterActorBox child_box;
  ClutterUnit child_wu, child_hu;
  gint child_w, child_h;

  /* chain up to set the allocation of the actor */
  CLUTTER_ACTOR_CLASS (penge_day_tile_parent_class)->allocate (actor, 
                                                               box, 
                                                               origin_changed);

  w = CLUTTER_UNITS_TO_DEVICE (box->x2 - box->x1);
  h = CLUTTER_UNITS_TO_DEVICE (box->y2 - box->y1);

  clutter_actor_get_preferred_width (priv->day_label, -1, NULL, &child_wu);
  clutter_actor_get_preferred_height (priv->day_label, -1, NULL, &child_hu);

  child_w = CLUTTER_UNITS_TO_DEVICE (child_wu);
  child_h = CLUTTER_UNITS_TO_DEVICE (child_hu);

  child_box.x1 = CLUTTER_UNITS_FROM_DEVICE (((w - child_w) / 2));
  child_box.y1 = CLUTTER_UNITS_FROM_DEVICE (PADDING);
  child_box.x2 = child_box.x1 + child_wu;
  child_box.y2 = child_box.y1 + child_hu;

  clutter_actor_allocate (priv->day_label, &child_box, origin_changed);

  clutter_actor_get_preferred_width (priv->date_label, -1, NULL, &child_wu);
  clutter_actor_get_preferred_height (priv->date_label, -1, NULL, &child_hu);

  child_w = CLUTTER_UNITS_TO_DEVICE (child_wu);
  child_h = CLUTTER_UNITS_TO_DEVICE (child_hu);

  child_box.x1 = CLUTTER_UNITS_FROM_DEVICE ((w - child_w) / 2);
  child_box.y1 = CLUTTER_UNITS_FROM_DEVICE (((h - child_h) / 2) + PADDING);
  child_box.x2 = child_box.x1 + child_wu;
  child_box.y2 = child_box.y1 + child_hu;

  clutter_actor_allocate (priv->date_label, &child_box, origin_changed);

  g_debug (G_STRLOC ": Allocate called.");
}


static void
penge_day_tile_class_init (PengeDayTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeDayTilePrivate));

  object_class->get_property = penge_day_tile_get_property;
  object_class->set_property = penge_day_tile_set_property;
  object_class->dispose = penge_day_tile_dispose;
  object_class->finalize = penge_day_tile_finalize;

  actor_class->allocate = _day_tile_allocate;
  actor_class->paint = _day_tile_paint;

  pspec = g_param_spec_object ("date",
                               "date",
                               "Date to show",
                               JANA_TYPE_TIME,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_TIME, pspec);
}

#define DAY_LABEL_FONT "Sans 12"
#define DATE_LABEL_FONT "Sans 32"
static void
penge_day_tile_init (PengeDayTile *self)
{
  PengeDayTilePrivate *priv = GET_PRIVATE (self);
  ClutterColor color_black = { 0, 0, 0, 255 };
  ClutterColor color_blue = { 0x4e, 0xb9, 0xef, 255 };

  priv->day_label = clutter_label_new_full (DAY_LABEL_FONT,
                                            "Unknown",
                                            &color_black);

  priv->date_label = clutter_label_new_full (DATE_LABEL_FONT,
                                            "XX",
                                            &color_blue);

  clutter_actor_set_parent (priv->day_label, self);
  clutter_actor_set_parent (priv->date_label, self);
}
