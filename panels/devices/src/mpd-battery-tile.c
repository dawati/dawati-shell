
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
#include "config.h"

G_DEFINE_TYPE (MpdBatteryTile, mpd_battery_tile, MX_TYPE_TABLE)

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
  char const            *icon_file = NULL;
  char                  *description = NULL;
  GError                *error = NULL;
  MpdBatteryDeviceState  state;
  int                    percentage;

  state = mpd_battery_device_get_state (priv->device);
  percentage = mpd_battery_device_get_percentage (priv->device);
  description = mpd_battery_device_get_state_text (priv->device);

  switch (state)
  {
  case MPD_BATTERY_DEVICE_STATE_MISSING:
    icon_file = PKGICONDIR "/dalston-power-missing.png";
    break;
  case MPD_BATTERY_DEVICE_STATE_CHARGING:
    icon_file = PKGICONDIR "/dalston-power-plugged.png";
    break;
  case MPD_BATTERY_DEVICE_STATE_DISCHARGING:
    if (percentage < 0)
      icon_file = PKGICONDIR "/dalston-power-missing.png";
    else if (percentage < 10)
      icon_file = PKGICONDIR "/dalston-power-0.png";
    else if (percentage < 30)
      icon_file = PKGICONDIR "/dalston-power-25.png";
    else if (percentage < 60)
      icon_file = PKGICONDIR "/dalston-power-50.png";
    else if (percentage < 90)
      icon_file = PKGICONDIR "/dalston-power-75.png";
    else
      icon_file = PKGICONDIR "/dalston-power-100.png";
    break;
  case MPD_BATTERY_DEVICE_STATE_FULLY_CHARGED:
    icon_file = PKGICONDIR "/dalston-power-100.png";
    break;
  default:
    icon_file = PKGICONDIR "/dalston-power-missing.png";
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

  mpd_gobject_detach (object, (GObject **) &priv->device);

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
  ClutterActor *text;

  mx_table_set_col_spacing (MX_TABLE (self), 24);

  priv->label = mx_label_new ("");
  text = mx_label_get_clutter_text (MX_LABEL (priv->label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (text), true);
  mx_table_add_actor_with_properties (MX_TABLE (self), priv->label,
                                      0, 0,
                                      "y-align", 0.5,
                                      "y-expand", false,
                                      "y-fill", false,
                                      NULL);

  priv->icon = clutter_texture_new ();
  /* Seems not to work huh
  clutter_texture_set_keep_aspect_ratio (CLUTTER_TEXTURE (priv->icon), true); */
  clutter_actor_set_height (priv->icon, 65.);
  clutter_actor_set_width (priv->icon, 65.);
  mx_table_add_actor (MX_TABLE (self), priv->icon, 0, 1);

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


