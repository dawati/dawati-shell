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


#include "mnp-alarms.h"

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
mnp_alarms_construct (MnpAlarms *alarms)
{
  MnpAlarmsPrivate *priv = ALARMS_PRIVATE(alarms);

  mx_box_layout_set_vertical ((MxBoxLayout *)alarms, FALSE);
  mx_box_layout_set_pack_start ((MxBoxLayout *)alarms, FALSE);
  
  priv->left_tiles = mx_box_layout_new();
  clutter_actor_set_name ((ClutterActor *)priv->left_tiles, "alarms-tile");
  mx_box_layout_set_vertical ((MxBoxLayout *)priv->left_tiles, FALSE);
  mx_box_layout_set_pack_start ((MxBoxLayout *)priv->left_tiles, FALSE);
  clutter_actor_set_size (priv->left_tiles, 300, 100);
  mx_box_layout_set_spacing ((MxBoxLayout *)priv->left_tiles, 3);

  priv->right_tiles = mx_box_layout_new();
  clutter_actor_set_size (priv->right_tiles, 100, 100);
  clutter_actor_set_name ((ClutterActor *)priv->right_tiles, "new-alarm-tile");
  mx_box_layout_set_vertical ((MxBoxLayout *)priv->right_tiles, FALSE);
  mx_box_layout_set_pack_start ((MxBoxLayout *)priv->right_tiles, FALSE);
  mx_box_layout_set_spacing ((MxBoxLayout *)priv->right_tiles, 3);

  clutter_container_add_actor ((ClutterContainer *)alarms, priv->left_tiles);
  clutter_container_child_set ((ClutterContainer *)alarms, priv->left_tiles,
                                   "x-fill", TRUE,
                                   "y-fill", TRUE,
                                   NULL);

  clutter_container_add_actor ((ClutterContainer *)alarms, priv->right_tiles);
  clutter_container_child_set ((ClutterContainer *)alarms, priv->right_tiles,
                                   "x-fill", FALSE,
                                   "y-fill", FALSE,
                                   NULL);
	
  clutter_actor_set_size((ClutterActor *)alarms, 430, 100);
}

MnpAlarms*
mnp_alarms_new (void)
{
  MnpAlarms *alarms = g_object_new (MNP_TYPE_ALARMS, NULL);

  mnp_alarms_construct(alarms);

  return alarms;
}
