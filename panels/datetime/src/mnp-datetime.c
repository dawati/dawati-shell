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

//#include "mnp-alarms.h"

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

	ClutterActor *cal_header;
	ClutterActor *task_header;

	ClutterActor *penge_events;
	ClutterActor *penge_tasks;

	ClutterActor *cal_date_label;
	ClutterActor *task_date_label;
	ClutterActor *top_date_label;

	ClutterActor *cal_launcher_box;
	ClutterActor *cal_launcher;

	ClutterActor *task_launcher_box;
	ClutterActor *task_launcher;

	guint date_update_timeout;
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

  if (priv->date_update_timeout)
	  g_source_remove(priv->date_update_timeout);

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
	char buf[256];

	localtime_r (&now, &tim);
	
	strftime (buf, 256, "%A %e %B %G", &tim);

	mx_label_set_text ((MxLabel *)label, buf);
}

static gboolean
events_pane_update (MnpDatetime *dtime)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (dtime);
 	JanaTime *start, *end;
	JanaDuration *duration;

	/*now = jana_ecal_utils_time_now_local ();
	g_object_set (priv->penge_events,
			"time",
			now,
			NULL);	
	g_object_unref (now); */

      	start = jana_ecal_utils_time_now_local ();
      	end = jana_ecal_utils_time_now_local ();
      	jana_time_set_hours (end, 23);
      	jana_time_set_minutes (end, 59);
      	jana_time_set_seconds (end, 59);

	duration = jana_duration_new (start, end);
	penge_events_pane_set_duration (priv->penge_events, duration);
	jana_duration_free (duration);


	return TRUE;
}

static void
construct_calendar_area (MnpDatetime *dtime)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (dtime);
/*	JanaTime *now; */
 	JanaTime *start, *end;
	JanaDuration *duration;
	ClutterActor *box, *label;
	ClutterActor *div, *icon;
  	
      	start = jana_ecal_utils_time_now_local ();
      	end = jana_ecal_utils_time_now_local ();
      	jana_time_set_hours (end, 23);
      	jana_time_set_minutes (end, 59);
      	jana_time_set_seconds (end, 59);

	
	duration = jana_duration_new (start, end);

	priv->cal_area = mx_box_layout_new ();
	clutter_actor_set_name (priv->cal_area, "CalendarPane");
	mx_box_layout_set_spacing ((MxBoxLayout *)priv->cal_area, 0);	
	mx_box_layout_set_orientation ((MxBoxLayout *)priv->cal_area, MX_ORIENTATION_VERTICAL);
	mx_box_layout_set_enable_animations ((MxBoxLayout *)priv->cal_area, TRUE);
	mx_box_layout_add_actor ((MxBoxLayout *)dtime, priv->cal_area, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (dtime),
                               priv->cal_area,
                               "expand", FALSE,
			       "y-fill", TRUE,		
			       "x-fill", FALSE,			       			       
                               NULL);
	clutter_actor_set_size (priv->cal_area, 350, -1);

	/* Events header */
	box = mx_box_layout_new ();
	clutter_actor_set_name (box, "EventsTitleBox");
	//mx_box_layout_set_spacing ((MxBoxLayout *)box, 20);
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
	mx_box_layout_add_actor ((MxBoxLayout *)priv->cal_area, (ClutterActor *)box, 0);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->cal_area),
                               box,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	

	icon = (ClutterActor *)mx_icon_new ();
  	mx_stylable_set_style_class (MX_STYLABLE (icon),
        	                       	"EventIcon");
  	clutter_actor_set_size (icon, 36, 36);
	mx_box_layout_add_actor (MX_BOX_LAYOUT(box), icon, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               icon,
			       "expand", FALSE,
			       "x-fill", FALSE,
			       "y-fill", FALSE,
			       "y-align", MX_ALIGN_MIDDLE,
			       "x-align", MX_ALIGN_START,
                               NULL);


	label = mx_label_new_with_text(_("Appointments"));
	clutter_actor_set_name (label, "EventPaneTitle");
	mx_box_layout_add_actor ((MxBoxLayout *)box, (ClutterActor *)label, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               label,
			       "expand", FALSE,
			       "x-fill", TRUE,
			       "y-fill", FALSE,			       
			       "y-align", MX_ALIGN_MIDDLE,
			       "x-align", MX_ALIGN_START,			       
                               NULL);

	/* Widgets header */
	box = mx_box_layout_new ();
	priv->cal_header = box;
	clutter_actor_set_name (box, "EventButtonBox");
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
	mx_box_layout_add_actor ((MxBoxLayout *)priv->cal_area, (ClutterActor *)box, 1);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->cal_area),
                               box,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	


	/* format date */
	box = mx_box_layout_new ();
	mx_box_layout_set_spacing ((MxBoxLayout *)box, 10);
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
	mx_box_layout_add_actor ((MxBoxLayout *)priv->cal_header, (ClutterActor *)box, 1);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->cal_header),
                               box,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	

	div = clutter_texture_new_from_file (SINGLE_DIV_LINE, NULL);
	mx_box_layout_add_actor ((MxBoxLayout *)priv->cal_area, div, 2);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->cal_area),
                               div,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	


	label = mx_label_new_with_text(_("Today"));
	clutter_actor_set_name (label, "CalendarPaneTitleToday");
	mx_box_layout_add_actor ((MxBoxLayout *)box, (ClutterActor *)label, 0 );
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               label,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", FALSE,			       			       
                               NULL);	
	
	priv->cal_date_label = mx_label_new ();
	clutter_actor_set_name (label, "CalendarPaneTitleDate");
	mx_box_layout_add_actor ((MxBoxLayout *)box, (ClutterActor *)priv->cal_date_label, 1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               priv->cal_date_label,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", FALSE,			       			       
                               NULL);	
	format_label (priv->cal_date_label);


	box = mx_box_layout_new ();
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_VERTICAL);
	mx_box_layout_add_actor ((MxBoxLayout *)priv->cal_area, box, 3);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->cal_area),
                               box,
                               "expand", TRUE,
			       "y-fill", TRUE,		
			       "x-fill", TRUE,			       			       
                               NULL);

  	priv->penge_events = g_object_new (PENGE_TYPE_EVENTS_PANE,
				    "time",
				    duration->start,
                                    NULL);
	penge_events_pane_set_duration (priv->penge_events, duration);
	jana_duration_free (duration);
	g_timeout_add_seconds (10, (GSourceFunc)events_pane_update, dtime);

	mx_box_layout_add_actor ((MxBoxLayout *)box, (ClutterActor *)priv->penge_events, 3);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               priv->penge_events,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);

	div = clutter_texture_new_from_file (SINGLE_DIV_LINE, NULL);
	mx_box_layout_add_actor (MX_BOX_LAYOUT(priv->cal_area), div, -1);		

	/* Launcher */
	box = mx_box_layout_new ();
	clutter_actor_set_name (box, "EventLauncherBox");
	priv->cal_launcher_box = box;
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_VERTICAL);
	mx_box_layout_set_spacing ((MxBoxLayout *)box, 6);
	
	priv->cal_launcher = mx_button_new ();
	mx_button_set_label ((MxButton *) priv->cal_launcher, _("Open Appointments"));
	mx_stylable_set_style_class (MX_STYLABLE(priv->cal_launcher), "EventLauncherButton");

	mx_box_layout_add_actor ((MxBoxLayout *)box, priv->cal_launcher, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               (ClutterActor *)priv->cal_launcher,
                               "expand", FALSE,
			       "y-fill", FALSE,
			       "x-fill", FALSE,
			       "x-align", MX_ALIGN_END,
                               NULL);

	mx_box_layout_add_actor ((MxBoxLayout *) priv->cal_area, box, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->cal_area),
                               (ClutterActor *)box,
                               "expand", FALSE,
			       "y-fill", FALSE,
			       "x-fill", TRUE,
                               NULL);	

}

static void
construct_task_area (MnpDatetime *time)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (time);
	ClutterActor *label, *div, *box, *icon;

	priv->task_row = mx_box_layout_new();
	clutter_actor_set_name (priv->task_row, "TaskPane");
	mx_box_layout_set_spacing ((MxBoxLayout *)priv->task_row, 0);	
	mx_box_layout_set_orientation ((MxBoxLayout *)priv->task_row, MX_ORIENTATION_VERTICAL);
	mx_box_layout_set_enable_animations ((MxBoxLayout *)priv->task_row, TRUE);
	mx_box_layout_add_actor ((MxBoxLayout *)time, priv->task_row, 4);
	clutter_container_child_set (CLUTTER_CONTAINER (time),
                               priv->task_row,
                               "expand", TRUE,
			       "y-fill", TRUE,		
			       "x-fill", TRUE,			       			       
                               NULL);

	box = mx_box_layout_new ();
	clutter_actor_set_name (box, "TasksTitleBox");
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
	mx_box_layout_add_actor ((MxBoxLayout *)priv->task_row, (ClutterActor *)box, 0);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->task_row),
                               box,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	

	icon = (ClutterActor *)mx_icon_new ();
  	mx_stylable_set_style_class (MX_STYLABLE (icon),
        	                       	"TaskIcon");
  	clutter_actor_set_size (icon, 36, 36);
	mx_box_layout_add_actor (MX_BOX_LAYOUT(box), icon, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               icon,
			       "expand", FALSE,
			       "x-fill", FALSE,
			       "y-fill", FALSE,
			       "y-align", MX_ALIGN_MIDDLE,
			       "x-align", MX_ALIGN_START,
                               NULL);


	label = mx_label_new_with_text (_("Tasks"));
	clutter_actor_set_name (label, "TaskPaneTitle");
	mx_box_layout_add_actor ((MxBoxLayout *)box, label, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               label,
			       "expand", FALSE,
			       "x-fill", TRUE,
			       "y-fill", FALSE,			       
			       "y-align", MX_ALIGN_MIDDLE,
			       "x-align", MX_ALIGN_START,			       
                               NULL);


	/* Widgets header */
	box = mx_box_layout_new ();
	priv->task_header = box;
	clutter_actor_set_name (box, "TaskButtonBox");
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
	mx_box_layout_add_actor ((MxBoxLayout *)priv->task_row, (ClutterActor *)box, 1);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->task_row),
                               box,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	


	/* format date */
	box = mx_box_layout_new ();
	mx_box_layout_set_spacing ((MxBoxLayout *)box, 10);
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
	mx_box_layout_add_actor ((MxBoxLayout *)priv->task_header, (ClutterActor *)box, 1);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->task_header),
                               box,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	

	div = clutter_texture_new_from_file (SINGLE_DIV_LINE, NULL);
	mx_box_layout_add_actor ((MxBoxLayout *)priv->task_row, div, 2);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->task_row),
                               div,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	


	label = mx_label_new_with_text(_("Today"));
	clutter_actor_set_name (label, "TaskPaneTitleToday");
	mx_box_layout_add_actor ((MxBoxLayout *)box, (ClutterActor *)label, 0 );
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               label,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", FALSE,			       			       
                               NULL);	
	
	priv->task_date_label = mx_label_new ();
	clutter_actor_set_name (label, "TaskPaneTitleDate");
	mx_box_layout_add_actor ((MxBoxLayout *)box, (ClutterActor *)priv->task_date_label, 1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               priv->task_date_label,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", FALSE,			       			       
                               NULL);	
	format_label (priv->task_date_label);


	box = mx_box_layout_new ();
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_VERTICAL);
	mx_box_layout_add_actor ((MxBoxLayout *)priv->task_row, box, 3);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->task_row),
                               box,
                               "expand", TRUE,
			       "y-fill", TRUE,		
			       "x-fill", TRUE,			       			       
                               NULL);

	priv->task_area = g_object_new (PENGE_TYPE_TASKS_PANE,
                                   NULL);
	mx_box_layout_add_actor ((MxBoxLayout *)box, priv->task_area, 3);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               priv->task_area,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);

	div = clutter_texture_new_from_file (SINGLE_DIV_LINE, NULL);
	mx_box_layout_add_actor (MX_BOX_LAYOUT(priv->task_row), div, -1);		

	/* Launcher */
	box = mx_box_layout_new ();
	clutter_actor_set_name (box, "TasksLauncherBox");
	priv->task_launcher_box = box;
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_VERTICAL);
	mx_box_layout_set_spacing ((MxBoxLayout *)box, 6);
	
	priv->task_launcher = mx_button_new ();
	mx_button_set_label ((MxButton *)priv->task_launcher, _("Open Tasks"));
	mx_stylable_set_style_class (MX_STYLABLE(priv->task_launcher), "TasksLauncherButton");

	mx_box_layout_add_actor ((MxBoxLayout *)box, priv->task_launcher, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               (ClutterActor *)priv->task_launcher,
                               "expand", FALSE,
			       "y-fill", FALSE,
			       "x-fill", FALSE,
			       "x-align", MX_ALIGN_END,
                               NULL);

	mx_box_layout_add_actor ((MxBoxLayout *)priv->task_row, box, -1);
	
}

static void
mnp_datetime_construct (MnpDatetime *time)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (time);

	mx_box_layout_set_orientation ((MxBoxLayout *)time, MX_ORIENTATION_HORIZONTAL);
	mx_box_layout_set_enable_animations ((MxBoxLayout *)time, TRUE);
	mx_box_layout_set_spacing ((MxBoxLayout *)time, 4);

	priv->world_clock = mnp_world_clock_new ();
	mx_box_layout_add_actor ((MxBoxLayout *) time, priv->world_clock, 0);
	clutter_container_child_set (CLUTTER_CONTAINER (time),
                               (ClutterActor *)priv->world_clock,
                               "expand", FALSE,
			       "y-fill", TRUE,
			       "x-fill", TRUE,
			       "y-align", MX_ALIGN_START,
                               NULL);
	
#if 0
	priv->alarm_area = (ClutterActor *)mnp_alarms_new();
	clutter_container_add_actor ((ClutterContainer *)priv->cal_alarm_row, (ClutterActor *)priv->alarm_area);
	
	div = clutter_texture_new_from_file (DOUBLE_DIV_LINE, NULL);
	clutter_container_add_actor ((ClutterContainer *)priv->cal_alarm_row, (ClutterActor *)div);
#endif	
	
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

static time_t
get_start_of_nextday (time_t now)
{
  struct tm tval;
  time_t start;

  localtime_r(&now, &tval);

  tval.tm_sec = 59;
  tval.tm_min = 59;
  tval.tm_hour = 23;

  start = mktime(&tval);
  start++;

  return start;
}

static gboolean
update_date (MnpDatetime *datetime)
{
  MnpDatetimePrivate *priv = GET_PRIVATE (datetime);
  time_t now = time(NULL), tom;

  tom = get_start_of_nextday (now);
  priv->date_update_timeout = g_timeout_add_seconds(tom-now+1, (GSourceFunc) update_date, datetime); 

  format_label (priv->top_date_label);
  format_label (priv->cal_date_label);
  format_label (priv->task_date_label);

  return FALSE;
}

void
mnp_date_time_set_date_label (MnpDatetime *datetime, ClutterActor *label)
{
  MnpDatetimePrivate *priv = GET_PRIVATE (datetime);
  time_t now = time(NULL), tom;

  priv->top_date_label = label;
  format_label (label);

  tom = get_start_of_nextday (now);

  priv->date_update_timeout = g_timeout_add_seconds(tom-now+1, (GSourceFunc) update_date, datetime);
}

