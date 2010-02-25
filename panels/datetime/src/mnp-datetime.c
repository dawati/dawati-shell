/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Srinivasa Ragavan <srini@linux.intel.com>
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

#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>


#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-entry.h>

#include "mnp-datetime.h"
#include "mnp-world-clock.h"
#include "mnp-alarms.h"

#include <time.h>

#include <libjana-ecal/jana-ecal.h>
#include <libjana/jana.h>
#include <penge/penge-events-pane.h>
#include <penge/penge-tasks-pane.h>

G_DEFINE_TYPE (MnpDatetime, mnp_datetime, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNP_TYPE_DATETIME, MnpDatetimePrivate))

#define V_DIV_LINE THEMEDIR "/v-div-line.png"
#define SINGLE_DIV_LINE THEMEDIR "/single-div-line.png"
#define DOUBLE_DIV_LINE THEMEDIR "/double-div-line.png"


typedef struct _MnpDatetimePrivate MnpDatetimePrivate;

struct _MnpDatetimePrivate {
	MplPanelClient *panel_client;

	ClutterActor *world_clock;
	ClutterActor *cal_alarm_row;
	ClutterActor *task_row;

	ClutterActor *alarm_area;
	ClutterActor *cal_area;
	ClutterActor *task_area;

	ClutterActor *penge_events;
	ClutterActor *penge_tasks;

	ClutterActor *date_label;
};

static void
mnp_datetime_dispose (GObject *object)
{
  MnpDatetimePrivate *priv = GET_PRIVATE (object);

  if (priv->panel_client)
  {
    g_object_unref (priv->panel_client);
    priv->panel_client = NULL;
  }

  G_OBJECT_CLASS (mnp_datetime_parent_class)->dispose (object);
}

static void
mnp_datetime_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnp_datetime_parent_class)->finalize (object);
}

static void
mnp_datetime_class_init (MnpDatetimeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnpDatetimePrivate));

  object_class->dispose = mnp_datetime_dispose;
  object_class->finalize = mnp_datetime_finalize;
}


static void
mnp_datetime_init (MnpDatetime *self)
{
  /* MnpDatetimePrivate *priv = GET_PRIVATE (self); */

}

static void
format_label (ClutterActor *label)
{
	time_t now = time(NULL);
	struct tm tim;
	char *buf[256];

	localtime_r (&now, &tim);
	
	strftime (buf, 256, "%d %b %G", &tim);

	mx_label_set_text (label, buf);
}

static void
construct_calendar_area (MnpDatetime *time)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (time);
  	JanaTime *start, *end;
	JanaDuration *duration;
	ClutterActor *box, *label;
	ClutterActor *div;
  	
      	start = jana_ecal_utils_time_now_local ();
      	end = jana_ecal_utils_time_now_local ();
      	jana_time_set_hours (end, 23);
      	jana_time_set_minutes (end, 59);
      	jana_time_set_seconds (end, 59);

	
	duration = jana_duration_new (start, end);

	priv->cal_area = mx_box_layout_new ();
	mx_stylable_set_style_class (priv->cal_area, "CalendarPane");
	mx_box_layout_set_spacing (priv->cal_area, 6);	
	mx_box_layout_set_vertical ((MxBoxLayout *)priv->cal_area, TRUE);
	mx_box_layout_set_pack_start ((MxBoxLayout *)priv->cal_area, FALSE);
	mx_box_layout_set_enable_animations ((MxBoxLayout *)priv->cal_area, TRUE);
	clutter_container_add_actor ((ClutterContainer *)priv->cal_alarm_row, priv->cal_area);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->cal_alarm_row),
                               priv->cal_area,
                               "expand", TRUE,
			       "y-fill", TRUE,		
			       "x-fill", TRUE,			       			       
                               NULL);
	/* format date */
	box = mx_box_layout_new ();
	mx_box_layout_set_spacing (box, 20);
	mx_box_layout_set_vertical ((MxBoxLayout *)box, FALSE);
	mx_box_layout_set_pack_start ((MxBoxLayout *)box, FALSE);
	clutter_container_add_actor ((ClutterContainer *)priv->cal_area, (ClutterActor *)box);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->cal_area),
                               box,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	

	div = clutter_texture_new_from_file (SINGLE_DIV_LINE, NULL);
	clutter_container_add_actor ((ClutterContainer *)priv->cal_area, div);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->cal_area),
                               div,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	


	label = mx_label_new(_("Today"));
	mx_stylable_set_style_class (label, "TodayLabel");
	clutter_container_add_actor ((ClutterContainer *)box, (ClutterActor *)label);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               label,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", FALSE,			       			       
                               NULL);	
	
	priv->date_label = mx_label_new (NULL);
	mx_stylable_set_style_class (label, "DateLabel");
	clutter_container_add_actor ((ClutterContainer *)box, (ClutterActor *)priv->date_label);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               priv->date_label,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", FALSE,			       			       
                               NULL);	
	format_label (priv->date_label);

  	priv->penge_events = g_object_new (PENGE_TYPE_EVENTS_PANE,
				    "time",
				    duration->start,
                                    NULL);
	penge_events_pane_set_duration (priv->penge_events, duration);
	jana_duration_free (duration);

	clutter_container_add_actor ((ClutterContainer *)priv->cal_area, (ClutterActor *)priv->penge_events);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->cal_area),
                               priv->penge_events,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);

}

static void
construct_task_area (MnpDatetime *time)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (time);
	ClutterActor *label, *div;

	priv->task_row = mx_box_layout_new();
	mx_box_layout_set_spacing ((MxBoxLayout *)priv->task_row, 4);	
	mx_box_layout_set_vertical ((MxBoxLayout *)priv->task_row, TRUE);
	mx_box_layout_set_pack_start ((MxBoxLayout *)priv->task_row, FALSE);
	mx_box_layout_set_enable_animations ((MxBoxLayout *)priv->task_row, TRUE);
	clutter_container_add_actor ((ClutterContainer *)time, priv->task_row);
	clutter_container_child_set (CLUTTER_CONTAINER (time),
                               priv->task_row,
                               "expand", TRUE,
			       "y-fill", TRUE,		
			       "x-fill", TRUE,			       			       
                               NULL);

	label = mx_label_new(_("Tasks"));
	mx_stylable_set_style_class (label, "TasksLabel");
	clutter_container_add_actor ((ClutterContainer *)priv->task_row, label);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->task_row),
                               label,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);

	div = clutter_texture_new_from_file (SINGLE_DIV_LINE, NULL);
	clutter_container_add_actor ((ClutterContainer *)priv->task_row, div);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->task_row),
                               div,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	

	priv->task_area = g_object_new (PENGE_TYPE_TASKS_PANE,
                                   NULL);
	clutter_container_add_actor ((ClutterContainer *)priv->task_row, priv->task_area);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->task_row),
                               priv->task_area,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);
}

static void
mnp_datetime_construct (MnpDatetime *time)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (time);
	ClutterActor *div;

	mx_box_layout_set_vertical ((MxBoxLayout *)time, FALSE);
	mx_box_layout_set_pack_start ((MxBoxLayout *)time, FALSE);
	mx_box_layout_set_enable_animations ((MxBoxLayout *)time, TRUE);
	mx_box_layout_set_spacing ((MxBoxLayout *)time, 4);
	clutter_actor_set_name ((ClutterActor *)time, "datetime-panel");

	priv->world_clock = mnp_world_clock_new ();
	clutter_container_add_actor ((ClutterContainer *)time, priv->world_clock);
	
	div = clutter_texture_new_from_file (V_DIV_LINE, NULL);
	clutter_container_add_actor ((ClutterContainer *)time, div);

	priv->cal_alarm_row = mx_box_layout_new();
	mx_box_layout_set_spacing ((MxBoxLayout *)priv->cal_alarm_row, 4);
	mx_box_layout_set_vertical ((MxBoxLayout *)priv->cal_alarm_row, TRUE);
	mx_box_layout_set_pack_start ((MxBoxLayout *)priv->cal_alarm_row, FALSE);
	mx_box_layout_set_enable_animations ((MxBoxLayout *)priv->cal_alarm_row, TRUE);
	clutter_container_add_actor ((ClutterContainer *)time, priv->cal_alarm_row);

	div = clutter_texture_new_from_file (V_DIV_LINE, NULL);
	clutter_container_add_actor ((ClutterContainer *)time, div);

	priv->alarm_area = (ClutterActor *)mnp_alarms_new();
	clutter_container_add_actor ((ClutterContainer *)priv->cal_alarm_row, (ClutterActor *)priv->alarm_area);
	
	div = clutter_texture_new_from_file (DOUBLE_DIV_LINE, NULL);
	clutter_container_add_actor ((ClutterContainer *)priv->cal_alarm_row, (ClutterActor *)div);
	
	construct_calendar_area(time);

	construct_task_area (time);



}

ClutterActor *
mnp_datetime_new (void)
{
  MnpDatetime *panel = g_object_new (MNP_TYPE_DATETIME, NULL);

  mnp_datetime_construct (panel);

  return (ClutterActor *)panel;
}

static void
dropdown_show_cb (MplPanelClient *client,
                  gpointer        userdata)
{
 /* MnpDatetimePrivate *priv = GET_PRIVATE (userdata); */

}


static void
dropdown_hide_cb (MplPanelClient *client,
                  gpointer        userdata)
{
 /* MnpDatetimePrivate *priv = GET_PRIVATE (userdata); */

}

void
mnp_datetime_set_panel_client (MnpDatetime *datetime,
                                   MplPanelClient *panel_client)
{
  MnpDatetimePrivate *priv = GET_PRIVATE (datetime);

  priv->panel_client = g_object_ref (panel_client);

  g_signal_connect (panel_client,
                    "show-end",
                    (GCallback)dropdown_show_cb,
                    datetime);

  g_signal_connect (panel_client,
                    "hide-end",
                    (GCallback)dropdown_hide_cb,
                    datetime);
}


