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

#include <config.h>
#include <glib/gi18n-lib.h>

#include "penge-calendar-pane.h"

#include <libjana-ecal/jana-ecal.h>
#include <libjana/jana.h>
#include <gio/gdesktopappinfo.h>
#include "penge-events-pane.h"
#include "penge-tasks-pane.h"
#include "penge-utils.h"

G_DEFINE_TYPE (PengeCalendarPane, penge_calendar_pane, MX_TYPE_WIDGET)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_CALENDAR_PANE, PengeCalendarPanePrivate))

#define GET_PRIVATE(o) ((PengeCalendarPane *)o)->priv
#define CALENDAR_ICON THEMEDIR "/calendar-icon-%d.png"


struct _PengeCalendarPanePrivate {
  ClutterActor *events_pane;
  ClutterActor *tasks_pane;

  guint refresh_timeout_id;
  guint8 day_of_month;

  ClutterActor *calendar_tex;
  ClutterActor *events_header_table;
  ClutterActor *tasks_header_table;
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
penge_calendar_pane_get_preferred_width (ClutterActor *actor,
                                         gfloat        for_height,
                                         gfloat       *min_width_p,
                                         gfloat       *nat_width_p)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (actor);
  gfloat header_nat, header_min;
  gfloat events_nat, events_min;
  gfloat tasks_nat, tasks_min;
  MxPadding padding = { 0, };

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  clutter_actor_get_preferred_width (CLUTTER_ACTOR (priv->events_header_table),
                                     for_height,
                                     &header_min,
                                     &header_nat);

  clutter_actor_get_preferred_width (CLUTTER_ACTOR (priv->events_pane),
                                     for_height,
                                     &events_min,
                                     &events_nat);

  clutter_actor_get_preferred_width (CLUTTER_ACTOR (priv->tasks_pane),
                                     for_height,
                                     &tasks_min,
                                     &tasks_nat);

  if (min_width_p)
    *min_width_p = MAX (header_min, MAX (events_min, tasks_min)) +
      padding.left + padding.right;

  if (nat_width_p)
    *nat_width_p = MAX (header_nat, MAX (events_nat, tasks_nat)) +
      padding.left + padding.right;
}

static void
penge_calendar_pane_get_preferred_height (ClutterActor *actor,
                                          gfloat        for_width,
                                          gfloat       *min_height_p,
                                          gfloat       *nat_height_p)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (actor);
  gfloat events_header_nat, events_header_min;
  gfloat tasks_header_nat, tasks_header_min;
  gfloat events_nat, events_min;
  gfloat tasks_nat, tasks_min;
  MxPadding padding = { 0, };

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->events_header_table),
                                      for_width,
                                      &events_header_min,
                                      &events_header_nat);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->events_pane),
                                      for_width,
                                      &events_min,
                                      &events_nat);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->tasks_header_table),
                                      for_width,
                                      &tasks_header_min,
                                      &tasks_header_nat);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->tasks_pane),
                                      for_width,
                                      &tasks_min,
                                      &tasks_nat);

  if (min_height_p)
    *min_height_p = events_header_min + events_min + tasks_header_min +
      tasks_min + padding.top + padding.bottom;

  if (nat_height_p)
    *nat_height_p = events_header_nat + events_nat + tasks_header_nat +
      tasks_nat + padding.top + padding.bottom;
}

static void
penge_calendar_pane_map (ClutterActor *actor)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (actor);

  if (CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->map)
    CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->map (actor);

  clutter_actor_map (CLUTTER_ACTOR (priv->events_header_table));
  clutter_actor_map (CLUTTER_ACTOR (priv->events_pane));
  clutter_actor_map (CLUTTER_ACTOR (priv->tasks_header_table));
  clutter_actor_map (CLUTTER_ACTOR (priv->tasks_pane));
}

static void
penge_calendar_pane_unmap (ClutterActor *actor)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (actor);

  if (CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->unmap)
    CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->unmap (actor);

  clutter_actor_unmap (CLUTTER_ACTOR (priv->events_header_table));
  clutter_actor_unmap (CLUTTER_ACTOR (priv->events_pane));
  clutter_actor_unmap (CLUTTER_ACTOR (priv->tasks_header_table));
  clutter_actor_unmap (CLUTTER_ACTOR (priv->tasks_pane));
}

static void
penge_calendar_pane_allocate (ClutterActor          *actor,
                              const ClutterActorBox *box,
                              ClutterAllocationFlags flags)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (actor);
  gfloat events_nat_h, tasks_nat_h, events_header_nat_h, tasks_header_nat_h;
  gfloat width, height, remaining_height, last_y;
  gfloat tasks_available_h, events_available_h;
  ClutterActorBox child_box;
  MxPadding padding = { 0, };

  if (CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->allocate)
    CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->allocate (actor, box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  width = box->x2 - box->x1;
  height = box->y2 - box->y1;

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->events_header_table),
                                      width,
                                      NULL,
                                      &events_header_nat_h);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->events_pane),
                                      width,
                                      NULL,
                                      &events_nat_h);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->tasks_header_table),
                                      width,
                                      NULL,
                                      &tasks_header_nat_h);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->tasks_pane),
                                      width,
                                      NULL,
                                      &tasks_nat_h);

  child_box.x1 = padding.left;
  child_box.x2 = width - padding.right;
  child_box.y1 = padding.top;
  child_box.y2 = child_box.y1 + events_header_nat_h;
  last_y = child_box.y2;
  clutter_actor_allocate (CLUTTER_ACTOR (priv->events_header_table),
                          &child_box,
                          flags);

  remaining_height = height - padding.top - padding.bottom -
                     events_header_nat_h - tasks_header_nat_h;

  if (tasks_nat_h <= (remaining_height / 2.0))
  {
    tasks_available_h = tasks_nat_h;
  } else {
    tasks_available_h = (remaining_height / 2.0);
  }

  events_available_h = remaining_height - tasks_available_h;
  if (events_available_h > events_nat_h)
  {
    tasks_available_h += (tasks_available_h - events_nat_h);
    events_available_h = events_nat_h;
  }

  child_box.x1 = padding.left;
  child_box.x2 = width - padding.right;
  child_box.y1 = last_y;
  child_box.y2 = child_box.y1 + (int)events_available_h;
  last_y = child_box.y2;

  clutter_actor_allocate (CLUTTER_ACTOR (priv->events_pane),
                          &child_box,
                          flags);

  child_box.x1 = padding.left;
  child_box.x2 = width - padding.right;
  child_box.y1 = last_y;
  child_box.y2 = child_box.y1 + tasks_header_nat_h;
  last_y = child_box.y2;
  clutter_actor_allocate (CLUTTER_ACTOR (priv->tasks_header_table),
                          &child_box,
                          flags);


  child_box.x1 = padding.left;
  child_box.x2 = width - padding.right;
  child_box.y1 = last_y;
  child_box.y2 = child_box.y1 + (int)tasks_available_h;

  clutter_actor_allocate (CLUTTER_ACTOR (priv->tasks_pane),
                          &child_box,
                          flags);

}

static void
penge_calendar_pane_paint (ClutterActor *actor)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (actor);

  if (CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->paint)
    CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->paint (actor);

  clutter_actor_paint (CLUTTER_ACTOR (priv->events_header_table));
  clutter_actor_paint (CLUTTER_ACTOR (priv->events_pane));
  clutter_actor_paint (CLUTTER_ACTOR (priv->tasks_header_table));
  clutter_actor_paint (CLUTTER_ACTOR (priv->tasks_pane));
}

static void
penge_calendar_pane_pick (ClutterActor       *actor,
                          const ClutterColor *pick_color)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (actor);

  if (CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->paint)
    CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->paint (actor);

  clutter_actor_paint (CLUTTER_ACTOR (priv->events_header_table));
  clutter_actor_paint (CLUTTER_ACTOR (priv->events_pane));
  clutter_actor_paint (CLUTTER_ACTOR (priv->tasks_header_table));
  clutter_actor_paint (CLUTTER_ACTOR (priv->tasks_pane));
}

static void
penge_calendar_pane_class_init (PengeCalendarPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeCalendarPanePrivate));

  object_class->get_property = penge_calendar_pane_get_property;
  object_class->set_property = penge_calendar_pane_set_property;
  object_class->dispose = penge_calendar_pane_dispose;
  object_class->finalize = penge_calendar_pane_finalize;

  actor_class->map = penge_calendar_pane_map;
  actor_class->unmap = penge_calendar_pane_unmap;
  actor_class->get_preferred_width = penge_calendar_pane_get_preferred_width;
  actor_class->get_preferred_height = penge_calendar_pane_get_preferred_height;
  actor_class->allocate = penge_calendar_pane_allocate;
  actor_class->paint = penge_calendar_pane_paint;
  actor_class->pick = penge_calendar_pane_pick;
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
_launch_desktop_file (ClutterActor *actor,
                      const gchar  *desktop_id)
{
  GDesktopAppInfo *app_info;
  const gchar *path;

  app_info = g_desktop_app_info_new (desktop_id);

  if (!app_info)
    return;

  path = g_desktop_app_info_get_filename (app_info);

  if (penge_utils_launch_for_desktop_file (actor, path))
    penge_utils_signal_activated (actor);

  g_object_unref (app_info);
}

static void
_tasks_open_button_clicked_cb (MxButton *button,
                               gpointer userdata)
{
  _launch_desktop_file ((ClutterActor *)button, "tasks.desktop");
}

static void
_events_open_button_clicked_cb (MxButton *button,
                                gpointer  userdata)
{
  _launch_desktop_file ((ClutterActor *)button, "evolution-calendar.desktop");
}

static void
penge_calendar_pane_init (PengeCalendarPane *self)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE_REAL (self);
  JanaTime *now;
  JanaTime *on_the_next_hour;
  glong next_timeout_seconds;
  ClutterActor *label;
  ClutterActor *tasks_icon;
  ClutterActor *button;

  self->priv = priv;

  now = jana_ecal_utils_time_now_local ();

  /* Events header */
  priv->events_header_table = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (priv->events_header_table), 8);
  mx_stylable_set_style_class (MX_STYLABLE (priv->events_header_table),
                               "PengeEventsPaneHeader");
  priv->calendar_tex = clutter_texture_new ();
  /* Need to fix the size to avoid being squashed */
  clutter_actor_set_size (priv->calendar_tex, 27, 28);

  mx_table_add_actor_with_properties (MX_TABLE (priv->events_header_table),
                                      priv->calendar_tex,
                                      0, 0,
                                      "x-expand", FALSE,
                                      "x-fill", FALSE,
                                      "y-expand", TRUE,
                                      "y-fill", FALSE,
                                      NULL);

  penge_calendar_pane_update_calendar_icon (self, now);

  label = mx_label_new_with_text (_("Appointments"));
  mx_stylable_set_style_class (MX_STYLABLE (label),
                               "PengeEventsPaneTitle");
  mx_table_add_actor_with_properties (MX_TABLE (priv->events_header_table),
                                      label,
                                      0, 1,
                                      "y-expand", TRUE,
                                      "y-fill", FALSE,
                                      "x-expand", TRUE,
                                      "x-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      NULL);
  button = mx_button_new_with_label (_("Open"));
  g_signal_connect (button,
                    "clicked",
                    (GCallback)_events_open_button_clicked_cb,
                    NULL);
  mx_table_add_actor_with_properties (MX_TABLE (priv->events_header_table),
                                      button,
                                      0, 2,
                                      "y-expand", TRUE,
                                      "y-fill", FALSE,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "x-expand", FALSE,
                                      NULL);
  /* Tasks header */
  priv->tasks_header_table = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (priv->tasks_header_table), 8);

  mx_stylable_set_style_class (MX_STYLABLE (priv->tasks_header_table),
                               "PengeTasksPaneHeader");
  tasks_icon = mx_icon_new ();
  clutter_actor_set_name (tasks_icon, "tasks-icon");

  mx_table_add_actor_with_properties (MX_TABLE (priv->tasks_header_table),
                                      tasks_icon,
                                      0, 0,
                                      "x-expand", FALSE,
                                      "x-fill", FALSE,
                                      "y-expand", TRUE,
                                      "y-fill", FALSE,
                                      NULL);

  label = mx_label_new_with_text (_("Tasks"));
  mx_stylable_set_style_class (MX_STYLABLE (label),
                               "PengeTasksPaneTitle");
  mx_table_add_actor_with_properties (MX_TABLE (priv->tasks_header_table),
                                      label,
                                      0, 1,
                                      "y-expand", TRUE,
                                      "y-fill", FALSE,
                                      "x-expand", TRUE,
                                      "x-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      NULL);
  button = mx_button_new_with_label (_("Open"));
  g_signal_connect (button,
                    "clicked",
                    (GCallback)_tasks_open_button_clicked_cb,
                    NULL);
  mx_table_add_actor_with_properties (MX_TABLE (priv->tasks_header_table),
                                      button,
                                      0, 2,
                                      "y-expand", TRUE,
                                      "y-fill", FALSE,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "x-expand", FALSE,
                                      NULL);

  priv->events_pane = g_object_new (PENGE_TYPE_EVENTS_PANE,
                                    "time",
                                    now,
                                    NULL);

  priv->tasks_pane = g_object_new (PENGE_TYPE_TASKS_PANE,
                                   NULL);


  clutter_actor_set_parent (CLUTTER_ACTOR (priv->events_header_table),
                            CLUTTER_ACTOR (self));
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->events_pane),
                            CLUTTER_ACTOR (self));
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->tasks_header_table),
                            CLUTTER_ACTOR (self));
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->tasks_pane),
                            CLUTTER_ACTOR (self));

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

