#include "penge-event-tile.h"

#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>

G_DEFINE_TYPE (PengeEventTile, penge_event_tile, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_EVENT_TILE, PengeEventTilePrivate))

typedef struct _PengeEventTilePrivate PengeEventTilePrivate;

struct _PengeEventTilePrivate {
  JanaEvent *event;
  JanaTime *time;
  JanaStore *store;

  NbtkWidget *time_label;
  NbtkWidget *summary_label;
  NbtkWidget *details_label;
};

enum
{
  PROP_0,
  PROP_EVENT,
  PROP_TIME,
  PROP_STORE
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
    case PROP_TIME:
      g_value_set_object (value, priv->time);
      break;
    case PROP_STORE:
      g_value_set_object (value, priv->store);
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
    case PROP_TIME:
      if (priv->time)
        g_object_unref (priv->time);

      priv->time = g_value_dup_object (value);

      penge_event_tile_update ((PengeEventTile *)object);
      break;
    case PROP_STORE:
      priv->store = g_value_dup_object (value);
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

  if (priv->time)
  {
    g_object_unref (priv->time);
    priv->time = NULL;
  }

  if (priv->store)
  {
    g_object_unref (priv->store);
    priv->store = NULL;
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

  pspec = g_param_spec_object ("time",
                               "The time now",
                               "The time now",
                               JANA_TYPE_TIME,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_TIME, pspec);

  pspec = g_param_spec_object ("store",
                               "The store.",
                               "The store this event came from.",
                               JANA_ECAL_TYPE_STORE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_STORE, pspec);
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

  if (priv->time)
  {
    t = jana_event_get_start (priv->event);
    if (jana_time_get_day (priv->time) != jana_time_get_day (t))
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
  clutter_actor_set_width ((ClutterActor *)priv->time_label,
                           50);
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

  priv->details_label = nbtk_label_new ("Details text");
  nbtk_widget_set_alignment (priv->details_label, 0, 0.5);
  nbtk_widget_set_style_class_name (priv->details_label,
                                    "PengeEventDetails");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (priv->details_label));
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
                        (ClutterActor *)priv->details_label,
                        1,
                        1);

  /* Make the time label span two rows */
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               (ClutterActor *)priv->time_label,
                               "row-span",
                               2,
                               NULL);

  /* 
   * Make the summary and detail labels consume the remaining horizontal
   * space
   */
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               (ClutterActor *)priv->summary_label,
                               "x-expand",
                               TRUE,
                               NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               (ClutterActor *)priv->details_label,
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
  gchar *details_str;
  JanaTime *t;

  if (!priv->event)
    return;

  if (priv->time)
  {
    t = jana_event_get_start (priv->event);
    if (jana_time_get_day (priv->time) == jana_time_get_day (t))
    {
      time_str = jana_utils_strftime (t, "%H:%M");
    } else {
      time_str = jana_utils_strftime (t, "%a");
    }

    if (jana_utils_time_compare (t, priv->time, FALSE) < 0)
    {
      nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (priv->time_label),
                                          "past");
    } else {
      nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (priv->time_label),
                                          NULL);
    }

    nbtk_label_set_text (NBTK_LABEL (priv->time_label), time_str);
    g_object_unref (t);
    g_free (time_str);
  }

  summary_str = jana_event_get_summary (priv->event);
  if (summary_str)
  {
    nbtk_label_set_text (NBTK_LABEL (priv->summary_label), summary_str);
    g_free (summary_str);
  }

  details_str = jana_event_get_location (priv->event);

  if (!details_str)
  {
    details_str = jana_event_get_description (priv->event);
  }

  if (!details_str)
  {
    nbtk_label_set_text (NBTK_LABEL (priv->details_label), "");

    /* 
     * If we fail to get some kind of description make the summary text
     * cover both rows in the tile
     */
    clutter_actor_hide (CLUTTER_ACTOR (priv->details_label));
    clutter_container_child_set (CLUTTER_CONTAINER (tile),
                                 (ClutterActor *)priv->summary_label,
                                 "row-span",
                                 2,
                                 NULL);
  } else {
    nbtk_label_set_text (NBTK_LABEL (priv->details_label), details_str);
    g_free (details_str);

    clutter_actor_show (CLUTTER_ACTOR (priv->details_label));
    clutter_container_child_set (CLUTTER_CONTAINER (tile),
                                 (ClutterActor *)priv->summary_label,
                                 "row-span",
                                 1,
                                 NULL);

  }
}

gchar *
penge_event_tile_get_uid (PengeEventTile *tile)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (tile);

  return jana_component_get_uid (JANA_COMPONENT (priv->event));
}
