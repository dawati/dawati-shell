#include "penge-event-tile.h"

#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>

G_DEFINE_TYPE (PengeEventTile, penge_event_tile, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_EVENT_TILE, PengeEventTilePrivate))

typedef struct _PengeEventTilePrivate PengeEventTilePrivate;

struct _PengeEventTilePrivate {
  JanaEvent *event;
  JanaTime *today;

  NbtkWidget *time_label;
  NbtkWidget *summary_label;
  NbtkWidget *location_label;
};

enum
{
  PROP_0,
  PROP_EVENT,
  PROP_TODAY
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
    case PROP_TODAY:
      g_value_set_object (value, priv->today);
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
    case PROP_TODAY:
      if (priv->today)
        g_object_unref (priv->today);

      priv->today = g_value_dup_object (value);

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

  if (priv->today)
  {
    g_object_unref (priv->today);
    priv->today = NULL;
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

  pspec = g_param_spec_object ("today",
                               "The day today",
                               "The day today",
                               JANA_TYPE_TIME,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_TODAY, pspec);
}

static gboolean
_enter_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (userdata);
  JanaTime *t;
  gchar *time_str;

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      "hover");

  t = jana_event_get_start (priv->event);
  time_str = jana_utils_strftime (t, "%H:%M");

  nbtk_label_set_text (NBTK_LABEL (priv->time_label), time_str);
  g_object_unref (t);
  g_free (time_str);

  return FALSE;
}

static gboolean
_leave_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (userdata);
  JanaTime *t;
  gchar *time_str;

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      NULL);

  if (priv->today)
  {
    t = jana_event_get_start (priv->event);
    if (jana_time_get_day (priv->today) != jana_time_get_day (t))
    {
      time_str = jana_utils_strftime (t, "%a");
      nbtk_label_set_text (NBTK_LABEL (priv->time_label), time_str);
      g_free (time_str);
    }
    g_object_unref (t);
  }

  return FALSE;
}

static void
penge_event_tile_init (PengeEventTile *self)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor *tmp_text;
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
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (priv->summary_label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), PANGO_ELLIPSIZE_END);
  clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text), PANGO_ALIGN_LEFT);

  priv->location_label = nbtk_label_new ("Location text");
  nbtk_widget_set_alignment (priv->location_label, 0, 0.5);
  nbtk_widget_set_style_class_name (priv->location_label,
                                    "PengeEventLocation");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (priv->location_label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), PANGO_ELLIPSIZE_END);
  clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text), PANGO_ALIGN_LEFT);

  /* Populate the table */
  nbtk_table_add_actor (NBTK_TABLE (self),
                        (ClutterActor *)priv->time_label,
                        0,
                        0);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               (ClutterActor *)priv->time_label,
                               "x-expand",
                               FALSE,
                               NULL);

  nbtk_table_add_actor (NBTK_TABLE (self),
                        (ClutterActor *)priv->summary_label,
                        0,
                        1);
  nbtk_table_add_actor (NBTK_TABLE (self),
                        (ClutterActor *)priv->location_label,
                        1,
                        1);

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

  g_signal_connect (self,
                    "enter-event",
                    (GCallback)_enter_event_cb,
                    self);
  g_signal_connect (self,
                    "leave-event",
                    (GCallback)_leave_event_cb,
                    self);
}

static void
penge_event_tile_update (PengeEventTile *tile)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (tile);
  gchar *time_str;
  gchar *summary_str;
  gchar *location_str;
  JanaTime *t;

  if (!priv->event)
    return;

  if (priv->today)
  {
    t = jana_event_get_start (priv->event);
    if (jana_time_get_day (priv->today) == jana_time_get_day (t))
    {
      time_str = jana_utils_strftime (t, "%H:%M");
    } else {
      time_str = jana_utils_strftime (t, "%a");
    }

    nbtk_label_set_text (NBTK_LABEL (priv->time_label), time_str);
    g_object_unref (t);
    g_free (time_str);
  }

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
