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

#include <glib/gi18n.h>
#include "mnp-alarms.h"
#include "mnp-alarm-tile.h"
#include "mnp-alarm-dialog.h"
#include <gconf/gconf-client.h>

G_DEFINE_TYPE (MnpAlarms, mnp_alarms, MX_TYPE_BOX_LAYOUT)

#define ALARMS_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNP_TYPE_ALARMS, MnpAlarmsPrivate))

typedef struct _MnpAlarmsPrivate MnpAlarmsPrivate;

struct _MnpAlarmsPrivate
{
	ClutterActor *left_tiles;
	ClutterActor *right_tiles;

	GList *alarm_tiles;
	ClutterActor *new_alarm_tile;
};

static void
mnp_alarms_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnp_alarms_parent_class)->dispose (object);
}

static void
mnp_alarms_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnp_alarms_parent_class)->finalize (object);
}

static void
mnp_alarms_class_init (MnpAlarmsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnpAlarmsPrivate));

  object_class->dispose = mnp_alarms_dispose;
  object_class->finalize = mnp_alarms_finalize;
}

static void
mnp_alarms_init (MnpAlarms *self)
{
}

static void
new_alarm_clicked (MxButton *button, MnpAlarms *alarms)
{
  mnp_alarm_dialog_new ();
}

static void
load_alarm_tile (MxButton *button, MnpAlarmTile *tile)
{
  MnpAlarmItem *item = mnp_alarm_tile_get_item(tile);
  MnpAlarmDialog *dialog;

  dialog = mnp_alarm_dialog_new();
  mnp_alarm_dialog_set_item (dialog, item);
}

static void
load_alarms (MnpAlarms *alarms)
{
  MnpAlarmsPrivate *priv = ALARMS_PRIVATE(alarms);
  GConfClient *client = gconf_client_get_default ();
  GSList *list, *tmp;
  MnpAlarmItem *item = g_new0(MnpAlarmItem, 1);

  if (priv->alarm_tiles) {
	  g_list_foreach (priv->alarm_tiles, (GFunc)clutter_actor_destroy, NULL);
	  g_list_free(priv->alarm_tiles);
	  priv->alarm_tiles = NULL;
  }
  list = gconf_client_get_list (client,"/apps/date-time-panel/alarms", GCONF_VALUE_STRING, NULL);
  if (g_slist_length(list) > 3)
	  clutter_actor_hide (priv->right_tiles);
  else
	  clutter_actor_show (priv->right_tiles);
  tmp = list;
  while(tmp) {
	char *data = (char *)tmp->data;
	MnpAlarmTile *tile = mnp_alarm_tile_new ();

	mx_stylable_set_style_class (MX_STYLABLE(tile), "alarm-tile");

	g_signal_connect (tile, "clicked", G_CALLBACK(load_alarm_tile), tile);
	sscanf(data, "%d %d %d %d %d %d %d %d", &item->id, &item->on_off, &item->hour, &item->minute, &item->am_pm, &item->repeat, &item->snooze, &item->sound);
	clutter_container_add_actor ((ClutterContainer *)priv->left_tiles, (ClutterActor *)tile);
	clutter_container_child_set ((ClutterContainer *)priv->left_tiles, (ClutterActor *)tile,
                                   "x-fill", FALSE,
                                   "y-fill", TRUE,
				   "expand", FALSE,				   
                                   NULL);

	mnp_alarm_tile_set_item (tile, item);
  	tmp = tmp->next;
	priv->alarm_tiles = g_list_append (priv->alarm_tiles, tile);
  }
  
  g_slist_foreach(list, (GFunc)g_free, NULL);
  g_slist_free(list);
  g_free(item);

  g_object_unref (client);
}

static void
alarms_changed (GConfClient *client,
		guint cnxn_id,
		GConfEntry *entry,
		gpointer user_data)
{
	MnpAlarms *alarms = (MnpAlarms *)user_data;

	load_alarms(alarms);
}

static void
setup_watcher (MnpAlarms *alarms)
{
  GConfClient *client = gconf_client_get_default ();

  gconf_client_add_dir (client, "/apps/date-time-panel", GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
  gconf_client_notify_add (client, "/apps/date-time-panel/alarms", alarms_changed, alarms, NULL, NULL);
	
}

static void
mnp_alarms_construct (MnpAlarms *alarms)
{
  MnpAlarmsPrivate *priv = ALARMS_PRIVATE(alarms);

  priv->alarm_tiles = NULL;

  mx_box_layout_set_orientation ((MxBoxLayout *)alarms, MX_ORIENTATION_HORIZONTAL);
  mx_box_layout_set_pack_start ((MxBoxLayout *)alarms, FALSE);
  
  priv->left_tiles = mx_box_layout_new();
  clutter_actor_set_name ((ClutterActor *)priv->left_tiles, "alarms-tile-area");
  mx_box_layout_set_orientation ((MxBoxLayout *)priv->left_tiles, MX_ORIENTATION_HORIZONTAL);
  mx_box_layout_set_pack_start ((MxBoxLayout *)priv->left_tiles, FALSE);
  mx_box_layout_set_spacing ((MxBoxLayout *)priv->left_tiles, 3);

  priv->right_tiles = mx_box_layout_new();
  clutter_actor_set_size (priv->right_tiles, 100, -1);
  clutter_actor_set_name ((ClutterActor *)priv->right_tiles, "new-alarm-tile-area");
  mx_box_layout_set_orientation ((MxBoxLayout *)priv->right_tiles, MX_ORIENTATION_HORIZONTAL);
  mx_box_layout_set_pack_start ((MxBoxLayout *)priv->right_tiles, FALSE);
  mx_box_layout_set_spacing ((MxBoxLayout *)priv->right_tiles, 3);

  clutter_container_add_actor ((ClutterContainer *)alarms, priv->left_tiles);
  clutter_container_child_set ((ClutterContainer *)alarms, priv->left_tiles,
                                   "x-fill", TRUE,
                                   "y-fill", TRUE,
				   "expand", TRUE,
                                   NULL);

  clutter_container_add_actor ((ClutterContainer *)alarms, priv->right_tiles);
  clutter_container_child_set ((ClutterContainer *)alarms, priv->right_tiles,
                                   "x-fill", FALSE,
                                   "y-fill", TRUE,
                                   NULL);
	
  clutter_actor_set_size((ClutterActor *)alarms, 430, 100);


  priv->new_alarm_tile = (ClutterActor *)mnp_alarm_tile_new ();
  clutter_container_add_actor ((ClutterContainer *)priv->right_tiles, priv->new_alarm_tile);
  clutter_container_child_set ((ClutterContainer *)priv->right_tiles, priv->new_alarm_tile,
                                   "x-fill", TRUE,
                                   "y-fill", TRUE,
				   "expand", TRUE,				   
                                   NULL);	  
  mnp_alarm_set_text ((MnpAlarmTile *)priv->new_alarm_tile, NULL, _("New\nAlarm"));
  g_signal_connect (priv->new_alarm_tile, "clicked", G_CALLBACK(new_alarm_clicked), alarms);
  mx_stylable_set_style_class (MX_STYLABLE(priv->new_alarm_tile), "new-alarm-tile");

  setup_watcher (alarms);
  load_alarms(alarms);
}

MnpAlarms*
mnp_alarms_new (void)
{
  MnpAlarms *alarms = g_object_new (MNP_TYPE_ALARMS, NULL);

  mnp_alarms_construct(alarms);

  return alarms;
}


