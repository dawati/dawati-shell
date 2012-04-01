
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
#include "mpd-brightness-tile.h"
#include "mpd-display-device.h"
#include "mpd-gobject.h"
#include "mpd-shell-defines.h"

G_DEFINE_TYPE (MpdBrightnessTile, mpd_brightness_tile, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_BRIGHTNESS_TILE, MpdBrightnessTilePrivate))

typedef struct
{
  MxSlider            *slider;
  MpdBatteryDevice    *battery;
  MpdDisplayDevice    *display;
} MpdBrightnessTilePrivate;

static void
update_display_brightness (MpdBrightnessTile *self);

static void
update_brightness_display (MpdBrightnessTile *self);

static MpdDisplayDeviceMode
get_display_device_mode (MpdBrightnessTile *self)
{
  MpdBrightnessTilePrivate *priv = GET_PRIVATE (self);
  MpdBatteryDeviceState battery_state;
  MpdDisplayDeviceMode  mode;

  battery_state = mpd_battery_device_get_state (priv->battery);
  if (MPD_BATTERY_DEVICE_STATE_CHARGING == battery_state ||
      MPD_BATTERY_DEVICE_STATE_FULLY_CHARGED == battery_state)
  {
    mode = MPD_DISPLAY_DEVICE_MODE_AC;
  } else {
    mode = MPD_DISPLAY_DEVICE_MODE_BATTERY;
  }

  return mode;
}

static void
_brightness_slider_value_notify_cb (MxSlider          *slider,
                                    GParamSpec        *pspec,
                                    MpdBrightnessTile *self)
{
  update_display_brightness (self);
}

static bool
_update_brightness_display_cb (MpdBrightnessTile  *self)
{
  update_brightness_display (self);

  return false;
}

static void
_battery_state_notify_cb (MpdBatteryDevice   *display,
                          GParamSpec         *pspec,
                          MpdBrightnessTile  *self)
{
  /* Update again 1 second later in case we're getting
   * this notification before the power-icon. */
  g_timeout_add_seconds (1, (GSourceFunc) _update_brightness_display_cb, self);
}

static void
_display_brightness_notify_cb (MpdDisplayDevice   *display,
                               GParamSpec         *pspec,
                               MpdBrightnessTile  *self)
{
  update_brightness_display (self);
}

static void
_dispose (GObject *object)
{
  MpdBrightnessTilePrivate *priv = GET_PRIVATE (object);

  mpd_gobject_detach (object, (GObject **) &priv->battery);
  mpd_gobject_detach (object, (GObject **) &priv->display);

  G_OBJECT_CLASS (mpd_brightness_tile_parent_class)->dispose (object);
}

static void
mpd_brightness_tile_class_init (MpdBrightnessTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdBrightnessTilePrivate));

  object_class->dispose = _dispose;
}

static void
mpd_brightness_tile_init (MpdBrightnessTile *self)
{
  MpdBrightnessTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor  *icon;

  /* Layout  */
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), MPD_TILE_ICON_SPACING);

  icon = mx_icon_new ();
  clutter_actor_set_name (icon, "brightness-off");
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (self),
                                              icon,
                                              -1,
                                              "y-fill", FALSE,
                                              NULL);

  priv->slider = (MxSlider *) mx_slider_new ();
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (self),
                                              (ClutterActor *) priv->slider,
                                              -1,
                                              "expand", TRUE,
                                              NULL);
  g_signal_connect (priv->slider, "notify::value",
                    G_CALLBACK (_brightness_slider_value_notify_cb), self);

  icon = mx_icon_new ();
  clutter_actor_set_name (icon, "brightness-on");
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (self),
                                              icon,
                                              -1,
                                              "y-fill", FALSE,
                                              NULL);

  /* Hook the logic */
  priv->display = mpd_display_device_new ();
  g_signal_connect (priv->display, "notify::brightness",
                    G_CALLBACK (_display_brightness_notify_cb), self);

  priv->battery = mpd_battery_device_new ();
  g_signal_connect (priv->battery, "notify::state",
                    G_CALLBACK (_battery_state_notify_cb), self);

  update_brightness_display (self);
}

ClutterActor *
mpd_brightness_tile_new (void)
{
  return g_object_new (MPD_TYPE_BRIGHTNESS_TILE, NULL);
}

static void
update_display_brightness (MpdBrightnessTile *self)
{
  MpdBrightnessTilePrivate *priv = GET_PRIVATE (self);
  float brightness;

  g_signal_handlers_disconnect_by_func (priv->display,
                                        _display_brightness_notify_cb,
                                        self);

  brightness = mx_slider_get_value (priv->slider);
  mpd_display_device_set_brightness (priv->display,
                                     brightness,
                                     get_display_device_mode (self));

  g_signal_connect (priv->display, "notify::brightness",
                    G_CALLBACK (_display_brightness_notify_cb), self);
}

static void
update_brightness_display (MpdBrightnessTile *self)
{
  MpdBrightnessTilePrivate *priv = GET_PRIVATE (self);
  float brightness;

  g_signal_handlers_disconnect_by_func (priv->slider,
                                        _brightness_slider_value_notify_cb,
                                        self);

  brightness = mpd_display_device_get_brightness (priv->display);
  if (brightness >= 0)
    mx_slider_set_value (priv->slider, brightness);
  else
    g_warning ("%s : Brightness is %.1f", G_STRLOC, brightness);

  g_signal_connect (priv->slider, "notify::value",
                    G_CALLBACK (_brightness_slider_value_notify_cb), self);
}

