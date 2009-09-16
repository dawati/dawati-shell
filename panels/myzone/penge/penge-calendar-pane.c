/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include "penge-calendar-pane.h"

#include <libjana-ecal/jana-ecal.h>
#include <libjana/jana.h>

#include <glib/gi18n.h>
#include "penge-date-tile.h"
#include "penge-events-pane.h"
#include "penge-tasks-pane.h"

G_DEFINE_TYPE (PengeCalendarPane, penge_calendar_pane, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_CALENDAR_PANE, PengeCalendarPanePrivate))

#define CALENDAR_ICON THEMEDIR "/calendar-icon-%d.png"
#define SINGLE_DIV_LINE THEMEDIR "/single-div-line.png"
#define DOUBLE_DIV_LINE THEMEDIR "/double-div-line.png"

typedef struct _PengeCalendarPanePrivate PengeCalendarPanePrivate;

struct _PengeCalendarPanePrivate {
    ClutterActor *events_pane;
    ClutterActor *tasks_pane;

    guint refresh_timeout_id;
    guint8 day_of_month;

    ClutterActor *calendar_tex;
    NbtkWidget *header_table;
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
  PengeCalendarPanePrivate *priv = GET_PRIVATE (object);

  if (priv->refresh_timeout_id)
  {
    g_source_remove (priv->refresh_timeout_id);
    priv->refresh_timeout_id = 0;
  }

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
penge_calendar_pane_update_calendar_icon (PengeCalendarPane *pane,
                                          JanaTime          *time)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (pane);
  GError *error = NULL;
  gchar *path = NULL;

  if (jana_time_get_day (time) != priv->day_of_month)
  {
    priv->day_of_month = jana_time_get_day (time);
    path = g_strdup_printf (CALENDAR_ICON, priv->day_of_month);
    clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->calendar_tex),
                                   path,
                                   &error);

    g_free (path);

    if (error)
    {
      g_warning (G_STRLOC ": Error setting path on calendar texture: %s",
                 error->message);
      g_clear_error (&error);
    }
  }
}

static void
penge_calendar_pane_update (PengeCalendarPane *pane)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (pane);
  JanaTime *now;

  now = jana_ecal_utils_time_now_local ();
  g_object_set (priv->events_pane,
                "time",
                now,
                NULL);
  penge_calendar_pane_update_calendar_icon (pane, now);
  g_object_unref (now);
}

static gboolean
_refresh_timeout_cb (gpointer userdata)
{
  penge_calendar_pane_update ((PengeCalendarPane *)userdata);

  return TRUE;
}

static gboolean
_first_refresh_timeout_cb (gpointer userdata)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (userdata);

  penge_calendar_pane_update ((PengeCalendarPane *)userdata);

  /* refresxh every ten minutes to handle timezone changes */
  priv->refresh_timeout_id = g_timeout_add_seconds (10 * 60,
                                                    _refresh_timeout_cb,
                                                    userdata);
  return FALSE;
}

static void
penge_calendar_pane_init (PengeCalendarPane *self)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (self);
  JanaTime *now;
  JanaTime *on_the_next_hour;
  glong next_timeout_seconds;
  ClutterActor *tex;
  NbtkWidget *label;
  GError *error = NULL;
  ClutterActor *tmp_text;

  now = jana_ecal_utils_time_now_local ();

  /* Title bit at the top */
  priv->header_table = nbtk_table_new ();
  priv->calendar_tex = clutter_texture_new ();
  nbtk_table_add_actor (NBTK_TABLE (priv->header_table),
                        priv->calendar_tex,
                        0,
                        0);
  /* Need to fix the size to avoid being squashed */
  clutter_actor_set_size (priv->calendar_tex, 30, 31);

  /* Use expand TRUE and fill FALSE to center valign with label */
  clutter_container_child_set (CLUTTER_CONTAINER (priv->header_table),
                               priv->calendar_tex,
                               "x-expand", TRUE,
                               "x-fill", FALSE,
                               "y-expand", TRUE,
                               "y-fill", FALSE,
                               NULL);

  penge_calendar_pane_update_calendar_icon (self, now);

  label = nbtk_label_new (_("<b>Appointments</b>"));
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (label));
  clutter_text_set_use_markup (CLUTTER_TEXT (tmp_text), TRUE);
  nbtk_widget_set_style_class_name (NBTK_WIDGET (label),
                                    "PengeCalendarPaneTitle");
  nbtk_table_add_actor (NBTK_TABLE (priv->header_table),
                        (ClutterActor *)label,
                        0,
                        1);

  /* Use expand TRUE and fill FALSE to center valign with icon */
  clutter_container_child_set (CLUTTER_CONTAINER (priv->header_table),
                               (ClutterActor *)label,
                               "y-expand", TRUE,
                               "y-fill", FALSE,
                               "x-expand", TRUE,
                               "x-fill", FALSE,
                               "x-align", 0.0,
                               NULL);

  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        priv->header_table,
                                        0, 0,
                                        "x-expand", TRUE,
                                        "y-fill", TRUE,
                                        "x-fill", TRUE,
                                        "y-expand", FALSE,
                                        NULL);

  tex = clutter_texture_new_from_file (SINGLE_DIV_LINE, &error);

  if (!tex)
  {
    g_warning (G_STRLOC ": Error loading single divider: %s",
               error->message);
    g_clear_error (&error);
  } else {
    nbtk_table_add_actor (NBTK_TABLE (self),
                          tex,
                          1,
                          0);
    clutter_container_child_set (CLUTTER_CONTAINER (self),
                                 tex,
                                 "y-expand", FALSE,
                                 NULL);
  }

  priv->events_pane = g_object_new (PENGE_TYPE_EVENTS_PANE,
                                    "time",
                                    now,
                                    NULL);

  nbtk_table_add_actor (NBTK_TABLE (self),
                        priv->events_pane,
                        2,
                        0);

  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               priv->events_pane,
                               "y-expand", TRUE,
                               "y-fill", FALSE,
                               "y-align", 0.0,
                               NULL);

  tex = clutter_texture_new_from_file (DOUBLE_DIV_LINE, &error);

  if (!tex)
  {
    g_warning (G_STRLOC ": Error loading double divider: %s",
               error->message);
    g_clear_error (&error);
  } else {
    nbtk_table_add_actor (NBTK_TABLE (self),
                          tex,
                          3,
                          0);
    clutter_container_child_set (CLUTTER_CONTAINER (self),
                                 tex,
                                 "y-expand", FALSE,
                                 NULL);
  }

  priv->tasks_pane = g_object_new (PENGE_TYPE_TASKS_PANE,
                                   NULL);
  nbtk_table_add_actor (NBTK_TABLE (self),
                        priv->tasks_pane,
                        4,
                        0);

 clutter_container_child_set (CLUTTER_CONTAINER (self),
                              priv->tasks_pane,
                              "y-expand", TRUE,
                              "y-fill", FALSE,
                              "y-align", 0.0,
                              NULL);

 nbtk_table_set_row_spacing (NBTK_TABLE (self), 2);

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

  /* We need to calculate how long we must wait before for our first update.
   * We do this by subtracting the current time from the next hour and then
   * finding the remainder. This remainder is the number of seconds until the
   * next 10 minute past the hour points.
   */
  on_the_next_hour = jana_ecal_utils_time_now_local ();
  jana_time_set_minutes (on_the_next_hour, 0);
  jana_time_set_seconds (on_the_next_hour, 0);

  jana_utils_time_adjust (on_the_next_hour,
                          0,
                          0,
                          0,
                          1,
                          0,
                          0);
  jana_utils_time_diff (now,
                        on_the_next_hour,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &next_timeout_seconds);

  priv->refresh_timeout_id =
    g_timeout_add_seconds (next_timeout_seconds % (60 * 10),
                           _first_refresh_timeout_cb,
                           self);

  g_object_unref (now);
  g_object_unref (on_the_next_hour);
}

