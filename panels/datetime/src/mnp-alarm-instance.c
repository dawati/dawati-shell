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

#include "mnp-alarm-instance.h"
#include <glib/gi18n.h>
#include<libnotify/notify.h>
#include <gconf/gconf-client.h>
#include <canberra.h>

G_DEFINE_TYPE (MnpAlarmInstance, mnp_alarm_instance, G_TYPE_OBJECT)

#define ALARM_INSTANCE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNP_TYPE_ALARM_INSTANCE, MnpAlarmInstancePrivate))

enum {
	ALARM_CHANGED,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

typedef struct _MnpAlarmInstancePrivate MnpAlarmInstancePrivate;

struct _MnpAlarmInstancePrivate
{
  MnpAlarmItem *item;
  time_t next_notification_time;
  gboolean repeat;
};

struct _instance_recur_table {
  gint id;
  const char *name;
} alarm_ins_recur_table [] = {
  { MNP_ALARM_NEVER, N_("Never") },
  { MNP_ALARM_EVERYDAY, N_("Everyday") },
  { MNP_ALARM_WORKWEEK, N_("Monday - Friday") },
  { MNP_ALARM_MONDAY, N_("Monday") },
  { MNP_ALARM_TUESDAY, N_("Tuesday") },
  { MNP_ALARM_WEDNESDAY, N_("Wednesday") },
  { MNP_ALARM_THURSDAY, N_("Thursday") },
  { MNP_ALARM_FRIDAY, N_("Friday") },
  { MNP_ALARM_SATURDAY, N_("Saturday") },
  { MNP_ALARM_SUNDAY, N_("Sunday") }
};

static void
mnp_alarm_instance_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnp_alarm_instance_parent_class)->dispose (object);
}

static void
mnp_alarm_instance_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnp_alarm_instance_parent_class)->finalize (object);
}

static void
mnp_alarm_instance_class_init (MnpAlarmInstanceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnpAlarmInstancePrivate));

  object_class->dispose = mnp_alarm_instance_dispose;
  object_class->finalize = mnp_alarm_instance_finalize;

  signals[ALARM_CHANGED] =
		g_signal_new ("alarm-changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MnpAlarmInstanceClass , alarm_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

}

static void
mnp_alarm_instance_init (MnpAlarmInstance *self)
{
}

static time_t
calculate_seconds (MnpAlarmItem *item)
{
  time_t secs=0;
  int hour = (item->hour == 12) ? 0 : item->hour;

  if (!item->am_pm)
	  secs+= (12*60*60);

  secs+= (hour*60*60);
  secs+= (item->minute*60);

  return secs;
}

time_t
get_start_of_day (time_t now)
{
  struct tm tval;
  time_t start;

  localtime_r(&now, &tval);

  tval.tm_sec = 0;
  tval.tm_min = 0;
  tval.tm_hour = 0;

  start = mktime(&tval);

  return start;
}

time_t
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

int
get_week_day (time_t now)
{
  struct tm tval;

  localtime_r(&now, &tval);

  return tval.tm_wday;

}

static void
manipulate_time (MnpAlarmInstance *alarm, MnpAlarmItem *item, time_t now)
{
  MnpAlarmInstancePrivate *priv = ALARM_INSTANCE_PRIVATE(alarm);

  if (!item->repeat) {
	gboolean secs;
	time_t start = get_start_of_day(now);

	secs = calculate_seconds(item);

	if ((start+secs) < now) /* No notification, and its already over and not repeatable*/
		priv->next_notification_time = 0;
	else
		priv->next_notification_time = start + secs - now;
	
  } else {
	int wday = get_week_day(now);
	int repeat = alarm_ins_recur_table[item->repeat].id;

	if (repeat & (1 << wday)) {
		time_t secs;
		time_t start = get_start_of_day(now);

		secs = calculate_seconds(item);
		if ((start+secs) > now) /* No notification, and its already over and not repeatable*/
			priv->next_notification_time = start + secs - now;
		else {
			int i=wday+1;
			int numdays=0;
		
			while ((repeat & (1 << i)) == 0) {
				i++; 
				if (i>6) {
					i=0;
				}
			}
			
			if (i > wday) {
				numdays = i-wday-1;
			} else 
				numdays = 6 - wday + i;
			
			start = get_start_of_nextday(now);
			start += numdays * 24 * 60 * 60;

			priv->next_notification_time = start + secs - now;
		}
	} else {
		int i=wday+1;
		int numdays=0;
		time_t start;

		time_t secs = calculate_seconds(item);

		while ((repeat & (1 << i)) == 0) {
			i++; 
			if (i>6) {
				i=0;
			}
		}
			
		if (i > wday) {
			numdays = i-wday-1;
		} else 
			numdays = 6 - wday + i;
			
		start = get_start_of_nextday(now);
		start += numdays * 24 * 60 * 60;

		priv->next_notification_time = start + secs - now;
	}
  }
  {
	  time_t next = now+priv->next_notification_time;
	  printf("Loading Alarm at %s", ctime(&next));
  }
}

void
mnp_alarm_instance_remanipulate (MnpAlarmInstance *alarm, time_t now)
{
  MnpAlarmInstancePrivate *priv = ALARM_INSTANCE_PRIVATE(alarm);

  manipulate_time(alarm, priv->item, now);
}

static void
mnp_alarm_instance_construct (MnpAlarmInstance *alarm, MnpAlarmItem *item, time_t now)
{
  MnpAlarmInstancePrivate *priv = ALARM_INSTANCE_PRIVATE(alarm);

  priv->item = item;
  priv->repeat = item->repeat ? TRUE : FALSE;
  manipulate_time (alarm, item, now);

}

MnpAlarmInstance*
mnp_alarm_instance_new (MnpAlarmItem *item, time_t now)
{
  MnpAlarmInstance *alarm = g_object_new (MNP_TYPE_ALARM_INSTANCE, NULL);
  
  mnp_alarm_instance_construct (alarm, item, now);

  return alarm;
}

time_t
mnp_alarm_instance_get_time (MnpAlarmInstance *alarm)
{
  MnpAlarmInstancePrivate *priv = ALARM_INSTANCE_PRIVATE(alarm);

  return priv->next_notification_time;
}

static void
show_notification(MnpAlarmInstance *alarm)
{
  MnpAlarmInstancePrivate *priv = ALARM_INSTANCE_PRIVATE(alarm);
  gboolean once = FALSE;
  NotifyNotification *notify;
  char *string;
  MnpAlarmItem *item = priv->item;

  if (!once) {
	  once = TRUE;
	  notify_init(_("Alarm Notify"));
  }

  string = g_strdup_printf(_("Alarm at %d:%.2d %s"), item->hour, item->minute, item->am_pm ? _("am") : _("pm"));  
  notify = notify_notification_new(_("Moblin Alarm Notify"), string,NULL,NULL);
  notify_notification_set_timeout(notify,10000);
  notify_notification_set_category(notify,_("AlarmNotifications"));

  notify_notification_set_urgency (notify,NOTIFY_URGENCY_CRITICAL);

  notify_notification_show(notify,NULL);
  g_free(string);
}

static void
alarm_del (MnpAlarmItem *item)
{
  GList *list, *tmp, *del_node;
  GConfClient *client;

  client = gconf_client_get_default();

  list = gconf_client_get_list (client,"/apps/date-time-panel/alarms", GCONF_VALUE_STRING, NULL);
  tmp = list;
  while(tmp) {
	char *data = (char *)tmp->data;
	int id, on_off, hour, min, am_pm, recur, snooze, sound;

	sscanf(data, "%d %d %d %d %d %d %d %d", &id, &on_off, &hour, &min, &am_pm, &recur, &snooze, &sound);

	if (id == item->id) {
		del_node = tmp;
		break;
	}
		
  	tmp = tmp->next;
  }

  if (del_node) {
  	list = g_slist_remove_link (list, del_node);
  	gconf_client_set_list(client,"/apps/date-time-panel/alarms", GCONF_VALUE_STRING, list, NULL);	
	g_free (del_node->data);
	g_slist_free_1 (del_node);
  }

  g_slist_foreach(list, (GFunc)g_free, NULL);
  g_slist_free(list);

  g_object_unref(client);
}

static void
play_music()
{
	static ca_context *notification = NULL;
	const char *file = "/usr/share/sounds/login.wav";

	if (!notification) {
		ca_context_create(&notification);
		ca_context_change_props(
			notification,
			CA_PROP_APPLICATION_NAME,
			"Moblin Alarm Notify",
			NULL);
	}
	ca_context_play(notification, 0,
	CA_PROP_MEDIA_FILENAME, file,
	NULL);
}

void
mnp_alarm_instance_raise (MnpAlarmInstance *alarm)
{
  MnpAlarmInstancePrivate *priv = ALARM_INSTANCE_PRIVATE(alarm);
  MnpAlarmItem *item = priv->item;

  show_notification(alarm);
  if (item->sound == MNP_SOUND_BEEP)
	  gdk_beep();
  else if(item->sound == MNP_SOUND_MUSIC) {
	  play_music();
  }
  if (!priv->repeat) {
	  /* raised alarm should be deleted */
	  alarm_del(priv->item);
  } else {
	  g_signal_emit (alarm, signals[ALARM_CHANGED], 0);			
  }
}
