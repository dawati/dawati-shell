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

#include <config.h>
#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>


#include <meego-panel/mpl-panel-clutter.h>
#include <meego-panel/mpl-panel-common.h>
#include <meego-panel/mpl-entry.h>

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

static gboolean update_date (MnpDatetime *datetime);


struct _MnpDatetimePrivate {
	MplPanelClient *panel_client;

	ClutterActor *world_clock;

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
	static const char *lenv = NULL;
	const char *fmt = "%A %e %B %G";

	if (!lenv) {
		lenv = g_getenv("LANG");
		if (lenv && strncmp(lenv, "en", 2) != 0)
			fmt = "%x";
	}
	localtime_r (&now, &tim);
	
	strftime (buf, 256, fmt, &tim);

	mx_label_set_text ((MxLabel *)label, buf);
}

#if 0 
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
#endif

/* From myzone */
static void
penge_calendar_pane_update (MnpDatetime *pane)
{
  MnpDatetimePrivate *priv = GET_PRIVATE (pane);
  JanaTime *now;

  now = jana_ecal_utils_time_now_local ();
  g_object_set (priv->penge_events,
                "time",
                now,
                NULL);
  g_object_unref (now);
}

static gboolean
_refresh_timeout_cb (gpointer userdata)
{
  penge_calendar_pane_update ((MnpDatetime *)userdata);

  return TRUE;
}

static gboolean
_first_refresh_timeout_cb (gpointer userdata)
{
  MnpDatetimePrivate *priv = GET_PRIVATE (userdata);

  penge_calendar_pane_update ((MnpDatetime *)userdata);

  /* refresxh every ten minutes to handle timezone changes */
  g_timeout_add_seconds (10 * 60,
			_refresh_timeout_cb,
			userdata);
  return FALSE;
}

/* End from myzone */

static void
launch_cal (ClutterActor *actor, MnpDatetime *dtime)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (dtime);

	if (priv->panel_client) {
		mpl_panel_client_hide (priv->panel_client);
		mpl_panel_client_launch_application (priv->panel_client, "/usr/bin/evolution  --express -c calendar");
	}
}

static void
construct_calendar_area (MnpDatetime *dtime)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (dtime);
#if 0
	JanaTime *start, *end;
	JanaDuration *duration;
#endif
	ClutterActor *box, *label;
	ClutterActor *div, *icon;
  
	JanaTime *now;
	JanaTime *on_the_next_hour;
	glong next_timeout_seconds;
	
	now = jana_ecal_utils_time_now_local ();

#if 0	
      	start = jana_ecal_utils_time_now_local ();
      	end = jana_ecal_utils_time_now_local ();
      	jana_time_set_hours (end, 23);
      	jana_time_set_minutes (end, 59);
      	jana_time_set_seconds (end, 59);
	
	duration = jana_duration_new (start, end);
#endif

	priv->cal_area = mx_table_new ();
	clutter_actor_set_name (priv->cal_area, "CalendarPane");
	mx_box_layout_add_actor ((MxBoxLayout *)dtime, priv->cal_area, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (dtime),
                               priv->cal_area,
                               "expand", TRUE,
			       "y-fill", TRUE,		
			       "x-fill", TRUE,			       			       
                               NULL);
	//clutter_actor_set_size (priv->cal_area, 345, -1);

	/* Events header */
	box = mx_box_layout_new ();
	clutter_actor_set_name (box, "EventsTitleBox");
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
   	mx_table_add_actor_with_properties (MX_TABLE (priv->cal_area),
                               box,
                               0, 0,
                               "x-expand", TRUE,
                               "y-expand", FALSE,
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
			       "expand", TRUE,
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
   	mx_table_add_actor_with_properties (MX_TABLE (priv->cal_area),
                               box,
                               1, 0,
                               "x-expand", TRUE,
			       "x-fill", TRUE,
                               "y-expand", FALSE,
			       "y-fill", FALSE,		
                               NULL);	


	/* format date */
	box = mx_box_layout_new ();
	mx_box_layout_set_spacing ((MxBoxLayout *)box, 10);
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
	mx_box_layout_add_actor ((MxBoxLayout *)priv->cal_header, (ClutterActor *)box, 1);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->cal_header),
                               box,
                               "expand", TRUE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	

/*	div = clutter_texture_new_from_file (SINGLE_DIV_LINE, NULL);
        mx_table_add_actor_with_properties (MX_TABLE (priv->cal_area),
                               div,
                               2, 0,
                               "x-expand", TRUE,
                               "y-expand", FALSE,
			       "x-fill", TRUE,			       			       
                               NULL);
        clutter_actor_set_height (div, 2);
*/
	label = mx_label_new_with_text(_("Today"));
	/* HACK: This is a fixed size element but ellipsizes in non english when font size/width is modified via CSS
	 * So just expand the size a bit */	
	clutter_text_set_ellipsize (mx_label_get_clutter_text(label), PANGO_ELLIPSIZE_NONE);
	clutter_text_set_line_wrap (mx_label_get_clutter_text(label), FALSE);	
	clutter_actor_set_width (mx_label_get_clutter_text(label), clutter_actor_get_width(label)+5);
	
	clutter_actor_set_name (label, "CalendarPaneTitleToday");
	mx_box_layout_add_actor ((MxBoxLayout *)box, (ClutterActor *)label, 0 );
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               label,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	
	
	priv->cal_date_label = mx_label_new ();

	clutter_actor_set_name (priv->cal_date_label, "CalendarPaneTitleDate");
	mx_box_layout_add_actor ((MxBoxLayout *)box, (ClutterActor *)priv->cal_date_label, 1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               priv->cal_date_label,
                               "expand", TRUE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	
	format_label (priv->cal_date_label);


  	priv->penge_events = g_object_new (PENGE_TYPE_EVENTS_PANE,
				    "time",
				    now,
//				    duration->start,
				    "multiline-summary", 
				    TRUE,
                                    NULL);
	mx_table_add_actor_with_properties (MX_TABLE (priv->cal_area),
                               priv->penge_events,
                               2, 0,
                               "x-expand", TRUE,
                               "y-expand", TRUE,
			       "y-fill", TRUE,		
			       "x-fill", TRUE,			       			       
                               NULL);


#if 0	
	penge_events_pane_set_duration (priv->penge_events, duration);
	jana_duration_free (duration);
	g_timeout_add_seconds (10, (GSourceFunc)events_pane_update, dtime);
#endif
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

    	g_timeout_add_seconds (next_timeout_seconds % (60 * 10),
                           	_first_refresh_timeout_cb,
                           	dtime);

  	g_object_unref (now);
  	g_object_unref (on_the_next_hour);

/*	
	div = clutter_texture_new_from_file (SINGLE_DIV_LINE, NULL);
        mx_table_add_actor_with_properties (MX_TABLE (priv->cal_area),
                               div,
                               3, 0,
                               "x-expand", TRUE,
                               "y-expand", FALSE,
			       "x-fill", TRUE,			       			       
                               NULL);
        clutter_actor_set_height (div, 2);
*/

	/* Launcher */
	box = mx_box_layout_new ();
	clutter_actor_set_name (box, "EventLauncherBox");
	priv->cal_launcher_box = box;
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_VERTICAL);
	mx_box_layout_set_spacing ((MxBoxLayout *)box, 6);
	
	priv->cal_launcher = mx_button_new ();
	mx_button_set_label ((MxButton *) priv->cal_launcher, _("Open Appointments"));
	mx_stylable_set_style_class (MX_STYLABLE(priv->cal_launcher), "EventLauncherButton");
	g_signal_connect (priv->cal_launcher, "clicked", G_CALLBACK(launch_cal), dtime);

	mx_box_layout_add_actor ((MxBoxLayout *)box, priv->cal_launcher, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               (ClutterActor *)priv->cal_launcher,
                               "expand", FALSE,
			       "y-fill", FALSE,
			       "x-fill", FALSE,
			       "x-align", MX_ALIGN_END,
                               NULL);

	mx_table_add_actor_with_properties (MX_TABLE (priv->cal_area),
                               box,
                               3, 0,
                               "x-expand", TRUE,
                               "y-expand", FALSE,
			       "x-fill", TRUE,			       			       
                               NULL);
	

}

static void
launch_tasks (ClutterActor *actor, MnpDatetime *dtime)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (dtime);

	if (priv->panel_client) {
		mpl_panel_client_hide (priv->panel_client);
		mpl_panel_client_launch_application (priv->panel_client, "/usr/bin/tasks");
	}
}

static void
construct_task_area (MnpDatetime *dtime)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (dtime);
	ClutterActor *label, *div, *box, *icon;

	priv->task_area = mx_table_new ();
	clutter_actor_set_name (priv->task_area, "TaskPane");
	//clutter_actor_set_size (priv->task_area, 345, -1);

	mx_box_layout_add_actor_with_properties ((MxBoxLayout *)dtime, priv->task_area, 4,
                                 "expand", TRUE,
  			         "y-fill", TRUE,		
 			         "x-fill", TRUE,			       			       
                                 NULL);

	box = mx_box_layout_new ();
	clutter_actor_set_name (box, "TasksTitleBox");
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
        mx_table_add_actor_with_properties (MX_TABLE (priv->task_area),
                               box,
                               0, 0,
                               "x-expand", TRUE,
                               "y-expand", FALSE,
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
			       "expand", TRUE,
			       "x-fill", TRUE,
			       "y-fill", FALSE,			       
			       "y-align", MX_ALIGN_MIDDLE,
			       "x-align", MX_ALIGN_START,			       
                               NULL);


	/* Widgets header */
	box = mx_box_layout_new ();
	priv->task_header = box;
	clutter_actor_set_name (box, "TaskButtonBox");
	mx_box_layout_set_spacing ((MxBoxLayout *)box, 10);
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
        mx_table_add_actor_with_properties (MX_TABLE (priv->task_area),
                               box,
                               1, 0,
                               "x-expand", TRUE,
			       "x-fill", TRUE,
                               "y-expand", FALSE,
			       "y-fill", FALSE,		
                               NULL);	


	/* format date */
	label = mx_label_new_with_text(_("Today"));
	/* HACK: This is a fixed size element but ellipsizes in non english when font size/width is modified via CSS
	 * So just expand the size a bit */	
	clutter_text_set_ellipsize (mx_label_get_clutter_text(label), PANGO_ELLIPSIZE_NONE);
	clutter_text_set_line_wrap (mx_label_get_clutter_text(label), FALSE);	
	clutter_actor_set_width (mx_label_get_clutter_text(label), clutter_actor_get_width(label)+5);
	clutter_actor_set_name (label, "TaskPaneTitleToday");
	mx_box_layout_add_actor ((MxBoxLayout *)box, (ClutterActor *)label, 0 );
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               label,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", FALSE,			       			       
                               NULL);	
	
	priv->task_date_label = mx_label_new ();
	clutter_actor_set_name (priv->task_date_label, "TaskPaneTitleDate");
	mx_box_layout_add_actor ((MxBoxLayout *)box, (ClutterActor *)priv->task_date_label, 1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               priv->task_date_label,
                               "expand", TRUE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	
	format_label (priv->task_date_label);

/*	div = clutter_texture_new_from_file (SINGLE_DIV_LINE, NULL);
        mx_table_add_actor_with_properties (MX_TABLE (priv->task_row),
                               div,
                               2, 0,
                               "x-expand", TRUE,
                               "y-expand", FALSE,
			       "x-fill", TRUE,			       			       
                               NULL);
        clutter_actor_set_height (div, 2);
*/
	priv->penge_tasks = g_object_new (PENGE_TYPE_TASKS_PANE,
                                   NULL);
        mx_table_add_actor_with_properties (MX_TABLE (priv->task_area),
                               priv->penge_tasks,
                               2, 0,
                               "x-expand", TRUE,
                               "y-expand", TRUE,
			       "y-fill", TRUE,		
			       "x-fill", TRUE,			       			       
                               NULL);

/*	div = clutter_texture_new_from_file (SINGLE_DIV_LINE, NULL);
        mx_table_add_actor_with_properties (MX_TABLE (priv->task_row),
                               div,
                               3, 0,
                               "x-expand", TRUE,
                               "y-expand", FALSE,
			       "x-fill", TRUE,			       			       
                               NULL);
        clutter_actor_set_height (div, 2);
*/
	/* Launcher */
	box = mx_box_layout_new ();
	clutter_actor_set_name (box, "TasksLauncherBox");
	priv->task_launcher_box = box;
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
	mx_box_layout_set_spacing ((MxBoxLayout *)box, 6);
        mx_table_add_actor_with_properties (MX_TABLE (priv->task_area),
                               box,
                               3, 0,
                               "x-expand", TRUE,
                               "y-expand", FALSE,
			       "x-fill", TRUE,			       			       
                               NULL);
	
	priv->task_launcher = mx_button_new ();
	mx_button_set_label ((MxButton *)priv->task_launcher, _("Open Tasks"));
	mx_stylable_set_style_class (MX_STYLABLE(priv->task_launcher), "TasksLauncherButton");
	g_signal_connect (priv->task_launcher, "clicked", G_CALLBACK(launch_tasks), dtime);
	mx_box_layout_add_actor ((MxBoxLayout *)box, priv->task_launcher, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               (ClutterActor *)priv->task_launcher,
                               "expand", TRUE,
			       "y-fill", FALSE,
			       "x-fill", FALSE,
			       "x-align", MX_ALIGN_END,
                               NULL);
}

static void
time_changed_now (MnpWorldClock *clock, MnpDatetime *dtime)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (dtime);
	
	g_source_remove (priv->date_update_timeout);
	update_date(dtime);
}

static void
mnp_datetime_construct (MnpDatetime *time)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (time);

	priv->panel_client = NULL;

	mx_box_layout_set_orientation ((MxBoxLayout *)time, MX_ORIENTATION_HORIZONTAL);
	mx_box_layout_set_enable_animations ((MxBoxLayout *)time, TRUE);
	mx_box_layout_set_spacing ((MxBoxLayout *)time, 4);

	priv->world_clock = mnp_world_clock_new ();
	g_signal_connect (priv->world_clock, "time-changed", G_CALLBACK(time_changed_now), time);

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

  mnp_world_clock_set_client (priv->world_clock, panel_client);

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

  /* format_label (priv->top_date_label); */
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
  /* format_label (label); */

  tom = get_start_of_nextday (now);

  priv->date_update_timeout = g_timeout_add_seconds(tom-now+1, (GSourceFunc) update_date, datetime);
}

