/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include "penge-event-tile.h"
#include "penge-utils.h"

#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>

#include <string.h>

G_DEFINE_TYPE (PengeEventTile, penge_event_tile, MX_TYPE_BUTTON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_EVENT_TILE, PengeEventTilePrivate))

typedef struct _PengeEventTilePrivate PengeEventTilePrivate;

struct _PengeEventTilePrivate {
  JanaEvent *event;
  JanaTime *time;
  JanaStore *store;

  ClutterActor *time_label;
  ClutterActor *summary_label;
  ClutterActor *details_label;
  ClutterActor *time_bin;

  ClutterActor *inner_table;
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

  t = jana_event_get_start (priv->event);

  /* Translate this time into local time */
  jana_time_set_offset (t, jana_time_get_offset (priv->time));

  time_str = jana_utils_strftime (t, "%H:%M");

  mx_label_set_text (MX_LABEL (priv->time_label), time_str);
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

  if (priv->time)
  {
    t = jana_event_get_start (priv->event);

    /* Translate this time into local time */
    jana_time_set_offset (t, jana_time_get_offset (priv->time));

    if (jana_time_get_day (priv->time) != jana_time_get_day (t))
    {
      time_str = jana_utils_strftime (t, "%a");
      mx_label_set_text (MX_LABEL (priv->time_label), time_str);
      g_free (time_str);
    }
    g_object_unref (t);
  }

  return FALSE;
}

static void
_button_clicked_cb (MxButton *button,
                    gpointer    userdata)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (userdata);
  ECal *ecal;
  gchar *uid;
  gchar *command_line;

  g_object_get (priv->store, "ecal", &ecal, NULL);
  uid = jana_component_get_uid ((JanaComponent *)priv->event);

  command_line = g_strdup_printf ("dates --edit-event \"%s %s\"",
                                  e_cal_get_uri (ecal),
                                  uid);
  g_free (uid);

  if (!penge_utils_launch_by_command_line ((ClutterActor *)button,
                                           command_line))
  {
    g_warning (G_STRLOC ": Error starting dates");
  } else{
    penge_utils_signal_activated ((ClutterActor *)userdata);
  }
}

static void
penge_event_tile_init (PengeEventTile *self)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor *tmp_text;

  priv->inner_table = mx_table_new ();
  mx_bin_set_child (MX_BIN (self), (ClutterActor *)priv->inner_table);
  mx_bin_set_fill (MX_BIN (self), TRUE, TRUE);

  priv->time_bin = mx_bin_new ();
  clutter_actor_set_width (priv->time_bin,
                           60);
  mx_widget_set_style_class_name (MX_WIDGET (priv->time_bin),
                                  "PengeEventTimeBin");

  priv->time_label = mx_label_new ("XX:XX");
  mx_widget_set_style_class_name (MX_WIDGET (priv->time_label),
                                  "PengeEventTimeLabel");
  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->time_label));

  mx_bin_set_child (MX_BIN (priv->time_bin),
                    priv->time_label);

  priv->summary_label = mx_label_new ("Summary text");
  mx_widget_set_style_class_name (MX_WIDGET (priv->summary_label),
                                  "PengeEventSummary");
  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->summary_label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), PANGO_ELLIPSIZE_END);
  clutter_text_set_single_line_mode (CLUTTER_TEXT (tmp_text), TRUE);

  priv->details_label = mx_label_new ("Details text");
  mx_widget_set_style_class_name (MX_WIDGET (priv->details_label),
                                  "PengeEventDetails");
  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->details_label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), PANGO_ELLIPSIZE_END);
  clutter_text_set_single_line_mode (CLUTTER_TEXT (tmp_text), TRUE);

  /* Populate the table */
  mx_table_add_actor (MX_TABLE (priv->inner_table),
                      priv->time_bin,
                      0,
                      0);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                               priv->time_bin,
                               "x-expand", FALSE,
                               "x-fill", FALSE,
                               "y-expand", FALSE,
                               "y-fill", FALSE,
                               NULL);

  mx_table_add_actor (MX_TABLE (priv->inner_table),
                      priv->summary_label,
                      0,
                      1);
  mx_table_add_actor (MX_TABLE (priv->inner_table),
                      priv->details_label,
                      1,
                      1);

  /* Make the time label span two rows */
  clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                               priv->time_bin,
                               "row-span",
                               2,
                               NULL);

  /* 
   * Make the summary and detail labels consume the remaining horizontal
   * space
   */
  clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                               priv->summary_label,
                               "x-expand", TRUE,
                               "y-fill", FALSE,
                               NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                               priv->details_label,
                               "x-expand", TRUE,
                               "y-fill", FALSE,
                               NULL);

  /* Setup spacing and padding */
  mx_table_set_row_spacing (MX_TABLE (priv->inner_table), 4);
  mx_table_set_col_spacing (MX_TABLE (priv->inner_table), 8);

  g_signal_connect (self,
                    "enter-event",
                    (GCallback)_enter_event_cb,
                    self);
  g_signal_connect (self,
                    "leave-event",
                    (GCallback)_leave_event_cb,
                    self);
  g_signal_connect (self,
                    "clicked",
                    (GCallback)_button_clicked_cb,
                    self);

  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);
}

static void
penge_event_tile_update (PengeEventTile *tile)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (tile);
  gchar *time_str;
  gchar *summary_str;
  gchar *details_str;
  JanaTime *t;
  gchar *p;

  if (!priv->event)
    return;

  if (priv->time)
  {
    t = jana_event_get_start (priv->event);

    /* Translate this time into local time */
    jana_time_set_offset (t, jana_time_get_offset (priv->time));

    if (jana_time_get_day (priv->time) == jana_time_get_day (t))
    {
      time_str = jana_utils_strftime (t, "%H:%M");
    } else {
      time_str = jana_utils_strftime (t, "%a");
    }

    if (jana_utils_time_compare (t, priv->time, FALSE) < 0)
    {
      mx_widget_set_style_pseudo_class (MX_WIDGET (priv->time_label),
                                        "past");
      mx_widget_set_style_pseudo_class (MX_WIDGET (priv->time_bin),
                                        "past");
    } else {
      mx_widget_set_style_pseudo_class (MX_WIDGET (priv->time_label),
                                        NULL);
      mx_widget_set_style_pseudo_class (MX_WIDGET (priv->time_bin),
                                        NULL);
    }

    mx_label_set_text (MX_LABEL (priv->time_label), time_str);
    g_object_unref (t);
    g_free (time_str);
  }

  summary_str = jana_event_get_summary (priv->event);
  if (summary_str)
  {
    /* this hack is courtesy of Chris Lord, we look for a new line character
     * and if we find it replace it with \0. We need this because otherwise
     * new lines in our labels look funn
     */
    p = strchr (summary_str, '\n');
    if (p)
      *p = '\0';
    mx_label_set_text (MX_LABEL (priv->summary_label), summary_str);
    g_free (summary_str);
  } else {
    mx_label_set_text (MX_LABEL (priv->summary_label), "");
  }

  details_str = jana_event_get_location (priv->event);

  if (!details_str)
  {
    details_str = jana_event_get_description (priv->event);
  }

  if (!details_str)
  {
    mx_label_set_text (MX_LABEL (priv->details_label), "");

    /* 
     * If we fail to get some kind of description make the summary text
     * cover both rows in the tile
     */
    clutter_actor_hide (CLUTTER_ACTOR (priv->details_label));
    clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                                 (ClutterActor *)priv->summary_label,
                                 "row-span",
                                 2,
                                 NULL);
  } else {
    p = strchr (details_str, '\n');
    if (p)
      *p = '\0';
    mx_label_set_text (MX_LABEL (priv->details_label), details_str);
    g_free (details_str);

    clutter_actor_show (CLUTTER_ACTOR (priv->details_label));
    clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
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
