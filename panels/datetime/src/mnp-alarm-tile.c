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

#include "mnp-alarm-tile.h"
#include "mnp-alarm-dialog.h"
#include <glib/gi18n.h>

G_DEFINE_TYPE (MnpAlarmTile, mnp_alarm_tile, MX_TYPE_BUTTON)

#define ALARM_TILE_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNP_TYPE_ALARM_TILE, MnpAlarmTilePrivate))

struct _recur_table {
  gint id;
  const char *name;
} alarm_tile_recur_table [] = {
  { MNP_ALARM_NEVER, NULL },
  { MNP_ALARM_EVERYDAY, N_("Everyday") },
  { MNP_ALARM_WORKWEEK, N_("MON-FRI") },
  { MNP_ALARM_MONDAY, N_("MONDAYS") },
  { MNP_ALARM_TUESDAY, N_("TUESDAYS") },
  { MNP_ALARM_WEDNESDAY, N_("WEDNESDAYS") },
  { MNP_ALARM_THURSDAY, N_("THURSDAYS") },
  { MNP_ALARM_FRIDAY, N_("FRIDAYS") },
  { MNP_ALARM_SATURDAY, N_("SATURDAYS") },
  { MNP_ALARM_SUNDAY, N_("SUNDAYS") }
};

typedef struct _MnpAlarmTilePrivate MnpAlarmTilePrivate;

struct _MnpAlarmTilePrivate
{
	ClutterActor *recur;
	ClutterActor *title;
	MnpAlarmItem *alarm_item;
};

static void
mnp_alarm_tile_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnp_alarm_tile_parent_class)->dispose (object);
}

static void
mnp_alarm_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnp_alarm_tile_parent_class)->finalize (object);
}

static void
mnp_alarm_tile_class_init (MnpAlarmTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnpAlarmTilePrivate));

  object_class->dispose = mnp_alarm_tile_dispose;
  object_class->finalize = mnp_alarm_tile_finalize;
}

static void
mnp_alarm_tile_init (MnpAlarmTile *self)
{
}

static void
mnp_alarm_tile_construct (MnpAlarmTile *btntile)
{
  MnpAlarmTilePrivate *priv = ALARM_TILE_PRIVATE(btntile);
  ClutterActor *tile;

  priv->alarm_item = NULL;

  clutter_actor_set_size (btntile, 100, -1);
  tile = mx_box_layout_new ();
  mx_box_layout_set_pack_start ((MxBoxLayout *)tile, FALSE);
  mx_box_layout_set_vertical ((MxBoxLayout *)tile, TRUE);
  mx_bin_set_fill (MX_BIN (btntile), TRUE, TRUE);
  mx_bin_set_alignment (MX_BIN (btntile), MX_ALIGN_MIDDLE, MX_ALIGN_MIDDLE);
  mx_bin_set_child (MX_BIN (btntile), (ClutterActor *)tile);

  priv->recur = mx_label_new (NULL);
  clutter_container_add_actor ((ClutterContainer *)tile, priv->recur);
  clutter_container_child_set ((ClutterContainer *)tile, priv->recur,
				   "expand", TRUE,
				   "y-fill", TRUE,
                                   NULL);

  priv->title = mx_label_new (NULL);
  clutter_container_add_actor ((ClutterContainer *)tile, priv->title);
  clutter_container_child_set ((ClutterContainer *)tile, priv->title,
				   "expand", TRUE,
				   "y-fill", TRUE,
                                   NULL);

}

MnpAlarmTile*
mnp_alarm_tile_new (void)
{
  MnpAlarmTile *tile = g_object_new (MNP_TYPE_ALARM_TILE, NULL);

  mnp_alarm_tile_construct (tile);

  return tile;
}

void
mnp_alarm_set_text (MnpAlarmTile *alarm, const char *recur, const char *title)
{
  MnpAlarmTilePrivate *priv = ALARM_TILE_PRIVATE(alarm);

  if (recur && *recur) {
	  mx_label_set_text ((MxLabel *)priv->recur, recur);
  } else
	  clutter_actor_hide (priv->recur);

  mx_label_set_text ((MxLabel *)priv->title, title);
}

static void
update_view(MnpAlarmTile *tile)
{
  MnpAlarmTilePrivate *priv = ALARM_TILE_PRIVATE(tile);
  char *txt;
  MnpAlarmItem *item = priv->alarm_item;

  if (alarm_tile_recur_table[priv->alarm_item->repeat].name) {
  	  mx_label_set_text ((MxLabel *)priv->recur, alarm_tile_recur_table[item->repeat].name);
	  clutter_actor_show(priv->recur);
  } else
	  clutter_actor_hide(priv->recur);

  txt = g_strdup_printf("%d:%.2d %s", item->hour, item->minute, item->am_pm ? "am" : "pm");
  mx_label_set_text ((MxLabel *)priv->title, txt);

  g_free(txt);
}

MnpAlarmItem *
mnp_alarm_item_copy (MnpAlarmItem *item)
{
  MnpAlarmItem *newitem = g_new0(MnpAlarmItem, 1);

  memcpy(newitem, item, sizeof(MnpAlarmItem));

  return newitem;
}

void
mnp_alarm_tile_set_item (MnpAlarmTile *tile, MnpAlarmItem *item)
{
  MnpAlarmTilePrivate *priv = ALARM_TILE_PRIVATE(tile);

  if(priv->alarm_item)
	  g_free(priv->alarm_item);
  priv->alarm_item = mnp_alarm_item_copy(item);

  update_view(tile);
}

MnpAlarmItem *
mnp_alarm_tile_get_item (MnpAlarmItem *tile)
{
  MnpAlarmTilePrivate *priv = ALARM_TILE_PRIVATE(tile);

  return priv->alarm_item;
}
