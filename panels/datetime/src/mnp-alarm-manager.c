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

#include "mnp-alarm-manager.h"
#include "mnp-alarm-utils.h"
#include "mnp-alarm-instance.h"
#include <gconf/gconf-client.h>

G_DEFINE_TYPE (MnpAlarmManager, mnp_alarm_manager, G_TYPE_OBJECT)

#define ALARM_MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNP_TYPE_ALARM_MANAGER, MnpAlarmManagerPrivate))

typedef struct _MnpAlarmManagerPrivate MnpAlarmManagerPrivate;

struct _MnpAlarmManagerPrivate
{
	guint timeout_source;

	GList *alarm_items;
	GList *alarm_instances;
};

static void load_alarms (MnpAlarmManager *man);

static void
mnp_alarm_manager_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnp_alarm_manager_parent_class)->dispose (object);
}

static void
mnp_alarm_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnp_alarm_manager_parent_class)->finalize (object);
}

static void
mnp_alarm_manager_class_init (MnpAlarmManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnpAlarmManagerPrivate));

  object_class->dispose = mnp_alarm_manager_dispose;
  object_class->finalize = mnp_alarm_manager_finalize;
}

static void
mnp_alarm_manager_init (MnpAlarmManager *self)
{
}

static void
alarms_changed (GConfClient *client,
		guint cnxn_id,
		GConfEntry *entry,
		gpointer user_data)
{
	MnpAlarmManager *alarms = (MnpAlarmManager *)user_data;

	load_alarms(alarms);
}

int
instance_sort_func (MnpAlarmInstance *a1, MnpAlarmInstance *a2, gpointer not_used)
{
  time_t t1, t2;

  t1 = mnp_alarm_instance_get_time(a1);
  t2 = mnp_alarm_instance_get_time(a2);

  return t1-t2;
}

static gboolean
mnp_alarm_handler(MnpAlarmManager *man)
{
  MnpAlarmManagerPrivate *priv = ALARM_MANAGER_PRIVATE(man);
  MnpAlarmInstance *alarm = (MnpAlarmInstance *) priv->alarm_instances->data;
  priv->timeout_source = 0;

  mnp_alarm_instance_raise (alarm);
  
  return FALSE;
}

static void
load_alarms (MnpAlarmManager *man)
{
  GConfClient *client;
  GSList *alarms, *tmp;
  MnpAlarmItem *item;
  MnpAlarmManagerPrivate *priv = ALARM_MANAGER_PRIVATE(man);
  time_t now = time(NULL);

  client = gconf_client_get_default();
  
  if (priv->alarm_items) {
	  g_list_foreach (priv->alarm_items, g_free, NULL);
	  g_list_free(priv->alarm_items);
	  priv->alarm_items = NULL;
	  g_list_foreach (priv->alarm_instances, g_object_unref, NULL);
	  g_list_free(priv->alarm_instances);
	  priv->alarm_instances = NULL;

  } else {
  	gconf_client_add_dir (client, "/apps/date-time-panel", GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
  	gconf_client_notify_add (client, "/apps/date-time-panel/alarms", alarms_changed, man, NULL, NULL);
  }
  
  alarms = gconf_client_get_list (client,"/apps/date-time-panel/alarms", GCONF_VALUE_STRING, NULL);
  tmp = alarms;
  while(tmp) {
	char *data = (char *)tmp->data;
	MnpAlarmInstance *instance;

	item = g_new0(MnpAlarmItem, 1);
	sscanf(data, "%d %d %d %d %d %d %d %d", &item->id, &item->on_off, &item->hour, &item->minute, &item->am_pm, &item->repeat, &item->snooze, &item->sound);
	priv->alarm_items = g_list_append(priv->alarm_items, item);

	instance = mnp_alarm_instance_new (item, now);
	priv->alarm_instances = g_list_insert_sorted_with_data(priv->alarm_instances, instance, (GCompareDataFunc)instance_sort_func, NULL);

	tmp = tmp->next;
  }

  g_slist_foreach(alarms, g_free, NULL);
  g_slist_free(alarms);
  g_object_unref(client);

  priv->timeout_source = g_timeout_add_seconds(mnp_alarm_instance_get_time((MnpAlarmInstance *)priv->alarm_instances->data), mnp_alarm_handler, man);
  now += mnp_alarm_instance_get_time((MnpAlarmInstance *)priv->alarm_instances->data);
  printf("\nWake up at %s", ctime(&now));
}

static void
mnp_alarm_manager_construct (MnpAlarmManager *man)
{
  MnpAlarmManagerPrivate *priv = ALARM_MANAGER_PRIVATE(man);

  /* init */
  priv->alarm_items = NULL;
  priv->alarm_instances = NULL;
  priv->timeout_source = 0;

  load_alarms(man);

}

MnpAlarmManager*
mnp_alarm_manager_new (void)
{
  MnpAlarmManager *man = g_object_new (MNP_TYPE_ALARM_MANAGER, NULL);

  mnp_alarm_manager_construct (man);

  return man;
}
