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

G_DEFINE_TYPE (MnpDatetime, mnp_datetime, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNP_TYPE_DATETIME, MnpDatetimePrivate))

#define TIMEOUT 250


typedef struct _MnpDatetimePrivate MnpDatetimePrivate;

struct _MnpDatetimePrivate {
	MplPanelClient *panel_client;

	ClutterActor *world_clock;
	ClutterActor *second_row;
	ClutterActor *alarm_area;
	ClutterActor *cal_area;
	ClutterActor *task_area;
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
mnp_datetime_construct (MnpDatetime *time)
{
  	MnpDatetimePrivate *priv = GET_PRIVATE (time);

	mx_box_layout_set_vertical ((MxBoxLayout *)time, FALSE);
	mx_box_layout_set_pack_start ((MxBoxLayout *)time, FALSE);
	mx_box_layout_set_enable_animations ((MxBoxLayout *)time, TRUE);
	mx_box_layout_set_spacing ((MxBoxLayout *)time, 3);
	clutter_actor_set_name ((ClutterActor *)time, "datetime-panel");

	priv->world_clock = mnp_world_clock_new ();
	clutter_container_add_actor ((ClutterContainer *)time, priv->world_clock);
	
	priv->second_row = mx_box_layout_new();
	mx_box_layout_set_vertical ((MxBoxLayout *)priv->second_row, TRUE);
	mx_box_layout_set_pack_start ((MxBoxLayout *)priv->second_row, FALSE);
	mx_box_layout_set_enable_animations ((MxBoxLayout *)priv->second_row, TRUE);
	clutter_container_add_actor ((ClutterContainer *)time, priv->second_row);

	priv->alarm_area = (ClutterActor *)mnp_alarms_new();
	clutter_container_add_actor ((ClutterContainer *)priv->second_row, (ClutterActor *)priv->alarm_area);
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


