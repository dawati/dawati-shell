
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdbool.h>

#include <glib/gi18n.h>

#include "mpd-battery-device.h"
#include "mpd-battery-tile.h"
#include "mpd-gobject.h"
#include "mpd-shell-defines.h"
#include "config.h"

G_DEFINE_TYPE (MpdBatteryTile, mpd_battery_tile, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_BATTERY_TILE, MpdBatteryTilePrivate))

enum
{
  SHOW_ME,

  LAST_SIGNAL
};

typedef struct
{
  /* Managed by Clutter. */
  ClutterActor      *progress_bar;

  /* Managed by man. */
  MpdBatteryDevice  *device;
} MpdBatteryTilePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static void
update (MpdBatteryTile *self)
{
  MpdBatteryTilePrivate *priv = GET_PRIVATE (self);
  MpdBatteryDeviceState  state;
  int                    percentage;
  gboolean               show_me;

  state = mpd_battery_device_get_state (priv->device);
  percentage = mpd_battery_device_get_percentage (priv->device);

  switch (state)
  {
  case MPD_BATTERY_DEVICE_STATE_MISSING:
    show_me = FALSE;
    break;
  case MPD_BATTERY_DEVICE_STATE_FULLY_CHARGED:
  case MPD_BATTERY_DEVICE_STATE_CHARGING:
  case MPD_BATTERY_DEVICE_STATE_DISCHARGING:
    show_me = TRUE;
    break;
  default:
    show_me = FALSE;
  }

  g_signal_emit (self, _signals[SHOW_ME], 0, show_me);

  mx_progress_bar_set_progress (MX_PROGRESS_BAR (priv->progress_bar),
                                percentage / 100.);
}

static void
_device_notify_cb (MpdBatteryDevice *battery,
                   GParamSpec       *pspec,
                   MpdBatteryTile   *self)
{
  update (self);
}

static void
_dispose (GObject *object)
{
  MpdBatteryTilePrivate *priv = GET_PRIVATE (object);

  mpd_gobject_detach (object, (GObject **) &priv->device);

  G_OBJECT_CLASS (mpd_battery_tile_parent_class)->dispose (object);
}

static void
mpd_battery_tile_class_init (MpdBatteryTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdBatteryTilePrivate));

  object_class->dispose = _dispose;

  _signals[SHOW_ME] = g_signal_new ("show-me",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    0, NULL, NULL,
                                    g_cclosure_marshal_VOID__BOOLEAN,
                                    G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

static void
mpd_battery_tile_init (MpdBatteryTile *self)
{
  MpdBatteryTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor *icon;

  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), MPD_TILE_ICON_SPACING);

  icon = mx_icon_new ();
  clutter_actor_set_name (icon, "battery-off");
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (self),
                                           icon,
                                           -1,
                                           "y-fill", FALSE,
                                           NULL);

  priv->progress_bar = mx_progress_bar_new ();
  clutter_actor_set_height (priv->progress_bar, 12);
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (self),
                                           priv->progress_bar,
                                           -1,
                                           "expand", TRUE,
                                           "y-fill", FALSE,
                                           NULL);

  icon = mx_icon_new ();
  clutter_actor_set_name (icon, "battery-on");
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (self),
                                           icon,
                                           -1,
                                           "y-fill", FALSE,
                                           NULL);

  priv->device = mpd_battery_device_new ();
  g_signal_connect (priv->device, "notify::percentage",
                    G_CALLBACK (_device_notify_cb), self);
  g_signal_connect (priv->device, "notify::state",
                    G_CALLBACK (_device_notify_cb), self);

  update (self);
}

ClutterActor *
mpd_battery_tile_new (void)
{
  return g_object_new (MPD_TYPE_BATTERY_TILE, NULL);
}
