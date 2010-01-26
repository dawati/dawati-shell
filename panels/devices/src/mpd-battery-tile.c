
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Robert Staudinger <robert.staudinger@intel.com>
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

#include <glib/gi18n.h>
#include "mpd-battery-device.h"
#include "mpd-battery-tile.h"
#include "config.h"

G_DEFINE_TYPE (MpdBatteryTile, mpd_battery_tile, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_BATTERY_TILE, MpdBatteryTilePrivate))

typedef struct
{
  /* Managed by Clutter. */
  ClutterActor      *label;
  ClutterActor      *icon;

  /* Managed by man. */
  MpdBatteryDevice  *device;
} MpdBatteryTilePrivate;

static void
update (MpdBatteryTile *self)
{
  MpdBatteryTilePrivate *priv = GET_PRIVATE (self);
  gchar const           *icon_file = NULL;
  gchar                 *description = NULL;
  GError                *error = NULL;
  MpdBatteryDeviceState  state;
  gint                   percentage;

  state = mpd_battery_device_get_state (priv->device);
  percentage = mpd_battery_device_get_percentage (priv->device);

  switch (state)
  {
  case MPD_BATTERY_DEVICE_STATE_MISSING:
    icon_file = ICONDIR "/dalston-power-missing.png";
    description = g_strdup_printf (_("Sorry, you don't appear to have a "
                                     "battery installed."));
    break;
  case MPD_BATTERY_DEVICE_STATE_CHARGING:
    icon_file = ICONDIR "/dalston-power-plugged.png";
    description = g_strdup_printf (_("Your battery is charging. "
                                     "It is about %d%% full."),
                                   percentage);
    break;
  case MPD_BATTERY_DEVICE_STATE_DISCHARGING:
    description = g_strdup_printf (_("Your battery is being used. "
                                     "It is about %d%% full."),
                                   percentage);
    break;
  case MPD_BATTERY_DEVICE_STATE_FULLY_CHARGED:
    icon_file = ICONDIR "/dalston-power-100.png";
    description = g_strdup_printf (_("Your battery is fully charged and "
                                     "you're ready to go."));
    break;
  default:
    icon_file = ICONDIR "/dalston-power-missing.png";
    description = g_strdup_printf (_("Sorry, it looks like your battery is "
                                     "broken."));
  }

  if (!icon_file)
  {
    if (percentage < 0)
      icon_file = ICONDIR "dalston-power-missing.png";
    else if (percentage < 10)
      icon_file = ICONDIR "dalston-power-0.png";
    else if (percentage < 30)
      icon_file = ICONDIR "dalston-power-25.png";
    else if (percentage < 60)
      icon_file = ICONDIR "dalston-power-50.png";
    else if (percentage < 90)
      icon_file = ICONDIR "dalston-power-75.png";
    else
      icon_file = ICONDIR "dalston-power-100.png";
  }

  mx_label_set_text (MX_LABEL (priv->label), description);
  g_free (description);

  clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->icon),
                                 icon_file, &error);
  if (error)
    g_warning ("%s : %s", G_STRLOC, error->message);
  g_clear_error (&error);
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

  if (priv->device)
  {
    g_object_unref (priv->device);
    priv->device = NULL;
  }

  G_OBJECT_CLASS (mpd_battery_tile_parent_class)->dispose (object);
}

static void
mpd_battery_tile_class_init (MpdBatteryTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdBatteryTilePrivate));

  object_class->dispose = _dispose;
}

static void
mpd_battery_tile_init (MpdBatteryTile *self)
{
  MpdBatteryTilePrivate *priv = GET_PRIVATE (self);

  priv->label = mx_label_new ("");
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->label);

  priv->icon = clutter_texture_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->icon);

  priv->device = mpd_battery_device_new ();
  g_signal_connect (priv->device, "notify::percentage",
                    G_CALLBACK (_device_notify_cb), self);
  g_signal_connect (priv->device, "notify::state",
                    G_CALLBACK (_device_notify_cb), self);

  update (self);
}

MpdBatteryTile *
mpd_battery_tile_new (void)
{
  return g_object_new (MPD_TYPE_BATTERY_TILE, NULL);
}


