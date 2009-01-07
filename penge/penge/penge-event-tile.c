#include "penge-event-tile.h"

#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>

G_DEFINE_TYPE (PengeEventTile, penge_event_tile, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_EVENT_TILE, PengeEventTilePrivate))

typedef struct _PengeEventTilePrivate PengeEventTilePrivate;

struct _PengeEventTilePrivate {
  JanaEvent *event;

  NbtkWidget *time_label;
  NbtkWidget *summary_label;
  NbtkWidget *location_label;
};

enum
{
  PROP_0,
  PROP_EVENT
};

static void penge_event_tile_update (PengeEventTile *tile);

static void
penge_event_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_EVENT:
      g_value_set_object (value, priv->event);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_event_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_EVENT:
      if (priv->event)
        g_object_unref (priv->event);

      priv->event = g_value_dup_object (value);

      penge_event_tile_update ((PengeEventTile *)object);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_event_tile_dispose (GObject *object)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (object);

  if (priv->event)
  {
    g_object_unref (priv->event);
    priv->event = NULL;
  }

  G_OBJECT_CLASS (penge_event_tile_parent_class)->dispose (object);
}

static void
penge_event_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_event_tile_parent_class)->finalize (object);
}

static void
penge_event_tile_class_init (PengeEventTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeEventTilePrivate));

  object_class->get_property = penge_event_tile_get_property;
  object_class->set_property = penge_event_tile_set_property;
  object_class->dispose = penge_event_tile_dispose;
  object_class->finalize = penge_event_tile_finalize;

  pspec = g_param_spec_object ("event",
                               "The event",
                               "The event to show details of",
                               JANA_TYPE_EVENT,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_EVENT, pspec);
}

static void
penge_event_tile_init (PengeEventTile *self)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor *label;
  NbtkPadding padding = { CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8) };

  priv->time_label = nbtk_label_new ("XX:XX");
  nbtk_widget_set_style_class_name (priv->time_label,
                                    "PengeEventTime");
  nbtk_widget_set_padding (priv->time_label, &padding);

  priv->summary_label = nbtk_label_new ("Summary text");
  nbtk_widget_set_style_class_name (priv->summary_label,
                                    "PengeEventSummary");
  nbtk_widget_set_alignment (priv->summary_label, 0, 0.5);
  label = nbtk_label_get_clutter_label (NBTK_LABEL (priv->summary_label));
  clutter_label_set_ellipsize (CLUTTER_LABEL (label), PANGO_ELLIPSIZE_END);
  clutter_label_set_alignment (CLUTTER_LABEL (label), PANGO_ALIGN_LEFT);

  priv->location_label = nbtk_label_new ("Location text");
  nbtk_widget_set_alignment (priv->location_label, 0, 0.5);
  nbtk_widget_set_style_class_name (priv->location_label,
                                    "PengeEventLocation");
  label = nbtk_label_get_clutter_label (NBTK_LABEL (priv->location_label));
  clutter_label_set_ellipsize (CLUTTER_LABEL (label), PANGO_ELLIPSIZE_END);
  clutter_label_set_alignment (CLUTTER_LABEL (label), PANGO_ALIGN_LEFT);

  /* Populate the table */
  nbtk_table_add_actor (NBTK_TABLE (self),
                        (ClutterActor *)priv->time_label,
                        0,
                        0);

  nbtk_table_add_actor (NBTK_TABLE (self),
                        (ClutterActor *)priv->summary_label,
                        0,
                        1);
  nbtk_table_add_actor (NBTK_TABLE (self),
                        (ClutterActor *)priv->location_label,
                        0,
                        2);

  /* Make the time label span two rows */
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               (ClutterActor *)priv->time_label,
                               "row-span",
                               2,
                               NULL);

  /* 
   * Make the summary and location labels consume the remaining horizontal
   * space
   */
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               (ClutterActor *)priv->summary_label,
                               "x-expand",
                               TRUE,
                               NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               (ClutterActor *)priv->location_label,
                               "x-expand",
                               TRUE,
                               NULL);

  /* Setup spacing and padding */
  nbtk_table_set_row_spacing (NBTK_TABLE (self), 4);
  nbtk_table_set_col_spacing (NBTK_TABLE (self), 8);

  nbtk_widget_set_padding (NBTK_WIDGET (self), &padding);
}

static void
penge_event_tile_update (PengeEventTile *tile)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (tile);
  gchar *time_str;
  gchar *summary_str;
  gchar *location_str;

  g_return_if_fail (priv->event != NULL);

  time_str = jana_utils_strftime (jana_event_get_start (priv->event), "%H:%M");
  nbtk_label_set_text (NBTK_LABEL (priv->time_label), time_str);
  g_free (time_str);

  summary_str = jana_event_get_summary (priv->event);
  nbtk_label_set_text (NBTK_LABEL (priv->summary_label), summary_str);
  g_free (summary_str);

  location_str = jana_event_get_location (priv->event);
  nbtk_label_set_text (NBTK_LABEL (priv->location_label), location_str);
  g_free (location_str);
}

gchar *
penge_event_tile_get_uid (PengeEventTile *tile)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (tile);

  return jana_component_get_uid (JANA_COMPONENT (priv->event));
}
