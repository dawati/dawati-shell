#include "penge-calendar-pane.h"

#include <libjana-ecal/jana-ecal.h>
#include <libjana/jana.h>

#include "penge-date-tile.h"
#include "penge-events-pane.h"

G_DEFINE_TYPE (PengeCalendarPane, penge_calendar_pane, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_CALENDAR_PANE, PengeCalendarPanePrivate))

typedef struct _PengeCalendarPanePrivate PengeCalendarPanePrivate;

struct _PengeCalendarPanePrivate {
    ClutterActor *date_tile;
    ClutterActor *events_pane;
};

static void
penge_calendar_pane_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_calendar_pane_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_calendar_pane_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_calendar_pane_parent_class)->dispose (object);
}

static void
penge_calendar_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_calendar_pane_parent_class)->finalize (object);
}

static void
penge_calendar_pane_class_init (PengeCalendarPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeCalendarPanePrivate));

  object_class->get_property = penge_calendar_pane_get_property;
  object_class->set_property = penge_calendar_pane_set_property;
  object_class->dispose = penge_calendar_pane_dispose;
  object_class->finalize = penge_calendar_pane_finalize;
}

static void
penge_calendar_pane_init (PengeCalendarPane *self)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (self);
  NbtkPadding padding = { CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8) };
  JanaTime *now;
  NbtkWidget *table;
  ClutterActor *padding_rectangle;

  now = jana_ecal_utils_time_now_local ();
  priv->date_tile = g_object_new (PENGE_TYPE_DATE_TILE,
                                  "time",
                                  now,
                                  NULL);
  clutter_actor_set_size (priv->date_tile, 150, 130);

  priv->events_pane = g_object_new (PENGE_TYPE_EVENTS_PANE,
                                    NULL);

  /* This in an enclosing table to let us set the background */
  table = nbtk_table_new ();
  nbtk_widget_set_style_class_name (table, "PengeDateTileBackground");
  nbtk_widget_set_padding (table, &padding);
  nbtk_table_add_actor (NBTK_TABLE (table),
                        priv->date_tile,
                        0,
                        0);
  clutter_container_child_set (CLUTTER_CONTAINER (table),
                               priv->date_tile,
                               "keep-aspect-ratio",
                               TRUE,
                               NULL);

  nbtk_table_add_actor (NBTK_TABLE (self),
                        (ClutterActor *)table,
                        0,
                        0);

  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               table,
                               "y-expand",
                               FALSE,
                               NULL);

  nbtk_table_add_actor (NBTK_TABLE (self),
                        priv->events_pane,
                        1,
                        0);

  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               priv->events_pane,
                               "y-expand",
                               FALSE,
                               NULL);

  nbtk_table_set_row_spacing (NBTK_TABLE (self), 8);
  nbtk_table_set_col_spacing (NBTK_TABLE (self), 8);

  nbtk_widget_set_padding (NBTK_WIDGET (self), &padding);
/*
  padding_rectangle = clutter_rectangle_new ();
  nbtk_table_add_actor (NBTK_TABLE (self),
                        padding_rectangle,
                        2,
                        0);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               padding_rectangle,
                               "y-expand",
                               TRUE,
                               NULL);
                               */
}

