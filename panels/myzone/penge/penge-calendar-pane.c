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

G_DEFINE_TYPE (PengeCalendarPane, penge_calendar_pane, NBTK_TYPE_WIDGET)

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

  ClutterActor *single_divider;
  ClutterActor *double_divider;
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
  NbtkPadding padding = { 0, };

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  clutter_actor_get_preferred_width (CLUTTER_ACTOR (priv->header_table),
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
  gfloat header_nat, header_min;
  gfloat events_nat, events_min;
  gfloat tasks_nat, tasks_min;
  gfloat single_div_nat, double_div_nat;
  NbtkPadding padding = { 0, };

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->header_table),
                                      for_width,
                                      &header_min,
                                      &header_nat);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->events_pane),
                                      for_width,
                                      &events_min,
                                      &events_nat);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->tasks_pane),
                                      for_width,
                                      &tasks_min,
                                      &tasks_nat);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->single_divider),
                                      for_width,
                                      NULL,
                                      &single_div_nat);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->double_divider),
                                      for_width,
                                      NULL,
                                      &double_div_nat);


  if (min_height_p)
    *min_height_p = header_min + events_min + tasks_min +
      padding.top + padding.bottom + single_div_nat + double_div_nat ;

  if (nat_height_p)
    *nat_height_p = header_nat + events_nat + tasks_nat +
      padding.top + padding.bottom + single_div_nat + double_div_nat;
}

static void
penge_calendar_pane_map (ClutterActor *actor)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (actor);

  if (CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->map)
    CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->map (actor);

  clutter_actor_map (CLUTTER_ACTOR (priv->header_table));
  clutter_actor_map (CLUTTER_ACTOR (priv->events_pane));
  clutter_actor_map (CLUTTER_ACTOR (priv->tasks_pane));
  clutter_actor_map (CLUTTER_ACTOR (priv->single_divider));
  clutter_actor_map (CLUTTER_ACTOR (priv->double_divider));
}

static void
penge_calendar_pane_unmap (ClutterActor *actor)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (actor);

  if (CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->unmap)
    CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->unmap (actor);

  clutter_actor_unmap (CLUTTER_ACTOR (priv->header_table));
  clutter_actor_unmap (CLUTTER_ACTOR (priv->events_pane));
  clutter_actor_unmap (CLUTTER_ACTOR (priv->tasks_pane));
  clutter_actor_unmap (CLUTTER_ACTOR (priv->single_divider));
  clutter_actor_unmap (CLUTTER_ACTOR (priv->double_divider));
}

static void
penge_calendar_pane_allocate (ClutterActor          *actor,
                              const ClutterActorBox *box,
                              ClutterAllocationFlags flags)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (actor);
  gfloat events_nat_h, tasks_nat_h, header_nat_h, single_div_nat_h, double_div_nat_h;
  gfloat width, height, remaining_height, last_y;
  gfloat tasks_available_h, events_available_h;
  ClutterActorBox child_box;
  NbtkPadding padding = { 0, };

  if (CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->allocate)
    CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->allocate (actor, box, flags);

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  width = box->x2 - box->x1;
  height = box->y2 - box->y1;

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->header_table),
                                      width,
                                      NULL,
                                      &header_nat_h);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->events_pane),
                                      width,
                                      NULL,
                                      &events_nat_h);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->tasks_pane),
                                      width,
                                      NULL,
                                      &tasks_nat_h);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->single_divider),
                                      width,
                                      NULL,
                                      &single_div_nat_h);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->double_divider),
                                      width,
                                      NULL,
                                      &double_div_nat_h);

  child_box.x1 = padding.left;
  child_box.x2 = width - padding.right;
  child_box.y1 = padding.top;
  child_box.y2 = child_box.y1 + header_nat_h;
  last_y = child_box.y2;
  clutter_actor_allocate (CLUTTER_ACTOR (priv->header_table),
                          &child_box,
                          flags);

  child_box.x1 = padding.left;
  child_box.x2 = width - padding.right;
  child_box.y1 = last_y;
  child_box.y2 = child_box.y1 + single_div_nat_h;
  last_y = child_box.y2;
  clutter_actor_allocate (CLUTTER_ACTOR (priv->single_divider),
                          &child_box,
                          flags);

  remaining_height = height - last_y - padding.bottom - double_div_nat_h;

  g_debug ("%f %f", remaining_height, events_nat_h + tasks_nat_h);

  if (tasks_nat_h <= (int)(remaining_height / 2.0))
  {
    tasks_available_h = tasks_nat_h;
  } else {
    tasks_available_h = (int)(remaining_height / 2.0);
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
  child_box.y2 = child_box.y1 + events_available_h;
  last_y = child_box.y2;

  clutter_actor_allocate (CLUTTER_ACTOR (priv->events_pane),
                          &child_box,
                          flags);

  child_box.x1 = padding.left;
  child_box.x2 = width - padding.right;
  child_box.y1 = last_y;
  child_box.y2 = child_box.y1 + double_div_nat_h;
  last_y = child_box.y2;

  clutter_actor_allocate (CLUTTER_ACTOR (priv->double_divider),
                          &child_box,
                          flags);

  child_box.x1 = padding.left;
  child_box.x2 = width - padding.right;
  child_box.y1 = last_y;
  child_box.y2 = child_box.y1 + tasks_available_h;
  last_y = child_box.y2;

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

  clutter_actor_paint (CLUTTER_ACTOR (priv->header_table));
  clutter_actor_paint (CLUTTER_ACTOR (priv->events_pane));
  clutter_actor_paint (CLUTTER_ACTOR (priv->tasks_pane));
  clutter_actor_paint (CLUTTER_ACTOR (priv->single_divider));
  clutter_actor_paint (CLUTTER_ACTOR (priv->double_divider));
}

static void
penge_calendar_pane_pick (ClutterActor       *actor,
                          const ClutterColor *pick_color)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (actor);

  if (CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->paint)
    CLUTTER_ACTOR_CLASS (penge_calendar_pane_parent_class)->paint (actor);

  clutter_actor_paint (CLUTTER_ACTOR (priv->header_table));
  clutter_actor_paint (CLUTTER_ACTOR (priv->events_pane));
  clutter_actor_paint (CLUTTER_ACTOR (priv->tasks_pane));
  clutter_actor_paint (CLUTTER_ACTOR (priv->single_divider));
  clutter_actor_paint (CLUTTER_ACTOR (priv->double_divider));
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
penge_calendar_pane_init (PengeCalendarPane *self)
{
  PengeCalendarPanePrivate *priv = GET_PRIVATE (self);
  JanaTime *now;
  JanaTime *on_the_next_hour;
  glong next_timeout_seconds;
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

  priv->single_divider = clutter_texture_new_from_file (SINGLE_DIV_LINE, &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error loading single divider: %s",
               error->message);
    g_clear_error (&error);
  }

  priv->double_divider = clutter_texture_new_from_file (DOUBLE_DIV_LINE, &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error loading double divider: %s",
               error->message);
    g_clear_error (&error);
  }

  priv->events_pane = g_object_new (PENGE_TYPE_EVENTS_PANE,
                                    "time",
                                    now,
                                    NULL);

  priv->tasks_pane = g_object_new (PENGE_TYPE_TASKS_PANE,
                                   NULL);


  clutter_actor_set_parent (CLUTTER_ACTOR (priv->header_table),
                            CLUTTER_ACTOR (self));
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->events_pane),
                            CLUTTER_ACTOR (self));
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->tasks_pane),
                            CLUTTER_ACTOR (self));
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->single_divider),
                            CLUTTER_ACTOR (self));
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->double_divider),
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

