
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

#include <gpm/gpm-brightness-xrandr.h>

#include "mpd-conf.h"
#include "mpd-display-device.h"
#include "mpd-gobject.h"

G_DEFINE_TYPE (MpdDisplayDevice, mpd_display_device, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_DISPLAY_DEVICE, MpdDisplayDevicePrivate))

enum
{
  PROP_0,

  PROP_ENABLED,
  PROP_BRIGHTNESS
};

typedef struct
{
  MpdConf             *conf;
  GpmBrightnessXRandR *brightness;
} MpdDisplayDevicePrivate;

static void
_brightness_changed_cb (GpmBrightnessXRandR *brightness,
                        unsigned int         percentage,
                        MpdDisplayDevice    *self)
{
  g_debug ("%s()", __FUNCTION__);
  g_object_notify (G_OBJECT (self), "brightness");
}

static void
_conf_brightness_enabled_notify_cb (MpdConf           *conf,
                                    GParamSpec        *pspec,
                                    MpdDisplayDevice  *self)
{
  g_object_notify (G_OBJECT (self), "enabled");
}

static void
_conf_brightness_value_notify_cb (MpdConf           *conf,
                                  GParamSpec        *pspec,
                                  MpdDisplayDevice  *self)
{
  g_object_notify (G_OBJECT (self), "brightness");
}

static void
_get_property (GObject      *object,
               unsigned int  property_id,
               GValue       *value,
               GParamSpec   *pspec)
{
  switch (property_id)
  {
  case PROP_ENABLED:
    g_value_set_boolean (value,
                         mpd_display_device_is_enabled (
                            MPD_DISPLAY_DEVICE (object)));
    break;
  case PROP_BRIGHTNESS:
    g_value_set_float (value,
                       mpd_display_device_get_brightness (
                          MPD_DISPLAY_DEVICE (object)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_set_property (GObject      *object,
               unsigned int  property_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  switch (property_id)
  {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (object);

  mpd_gobject_detach (object, (GObject **) &priv->conf);

  mpd_gobject_detach (object, (GObject **) &priv->brightness);

  G_OBJECT_CLASS (mpd_display_device_parent_class)->dispose (object);
}

static void
mpd_display_device_class_init (MpdDisplayDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamFlags   param_flags;

  g_type_class_add_private (klass, sizeof (MpdDisplayDevicePrivate));

  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
  object_class->dispose = _dispose;

  /* Properties */

  param_flags = G_PARAM_READABLE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_ENABLED,
                                   g_param_spec_boolean ("enabled",
                                                         "Enabled",
                                                         "If display brightness is enabled",
                                                         false,
                                                         param_flags));

  /* Read-only because we can not transfer the battery mode through
   * the property system. Use helper methods to set this. */
  g_object_class_install_property (object_class,
                                   PROP_BRIGHTNESS,
                                   g_param_spec_float ("brightness",
                                                       "Brightness",
                                                       "Display brightess 0 .. 1",
                                                       -1, 1, 1,
                                                       param_flags));
}

static void
mpd_display_device_init (MpdDisplayDevice *self)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);

  priv->conf = mpd_conf_new ();
  g_signal_connect (priv->conf, "notify::brightness-enabled",
                    G_CALLBACK (_conf_brightness_enabled_notify_cb), self);
  g_signal_connect (priv->conf, "notify::brightness-value",
                    G_CALLBACK (_conf_brightness_value_notify_cb), self);
  g_signal_connect (priv->conf, "notify::brightness-value-battery",
                    G_CALLBACK (_conf_brightness_value_notify_cb), self);

  priv->brightness = gpm_brightness_xrandr_new ();
  g_signal_connect (priv->brightness, "brightness-changed",
                    G_CALLBACK (_brightness_changed_cb), self);
}

MpdDisplayDevice *
mpd_display_device_new (void)
{
  return g_object_new (MPD_TYPE_DISPLAY_DEVICE, NULL);
}

bool
mpd_display_device_is_enabled (MpdDisplayDevice  *self)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);

  return mpd_conf_get_brightness_enabled (priv->conf);
}

float
mpd_display_device_get_brightness (MpdDisplayDevice  *self)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);
  unsigned int  percentage;

  g_return_val_if_fail (MPD_IS_DISPLAY_DEVICE (self), -1);

  if (gpm_brightness_xrandr_get (priv->brightness, &percentage))
  {
    return percentage / 100.0;
  } else
  {
    g_warning ("%s : gpm_brightness_xrandr_get() failed", G_STRLOC);
  }

  return -1;
}

static void
update_stored_brightness (MpdDisplayDevice      *self,
                          float                  brightness,
                          MpdDisplayDeviceMode   mode)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);

  g_signal_handlers_block_by_func (priv->conf,
                                   _conf_brightness_value_notify_cb,
                                   self);

  if (MPD_DISPLAY_DEVICE_MODE_AC == mode)
  {
    mpd_conf_set_brightness_value (priv->conf, brightness);
  } else {
    mpd_conf_set_brightness_value_battery (priv->conf, brightness);
  }

  g_signal_handlers_unblock_by_func (priv->conf,
                                     _conf_brightness_value_notify_cb,
                                     self);
}

void
mpd_display_device_set_brightness (MpdDisplayDevice     *self,
                                   float                 brightness,
                                   MpdDisplayDeviceMode  mode)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);
  gboolean hw_changed;

  g_return_if_fail (MPD_IS_DISPLAY_DEVICE (self));

  brightness = CLAMP (brightness, 0.0, 1.0);

  g_signal_handlers_block_by_func (priv->brightness,
                                   _brightness_changed_cb,
                                   self);

  if (gpm_brightness_xrandr_set (priv->brightness, brightness * 100, &hw_changed))
  {
    update_stored_brightness (self, brightness, mode);
  } else {
    g_warning ("%s : Setting brightness failed", G_STRLOC);
  }

  g_signal_handlers_unblock_by_func (priv->brightness,
                                     _brightness_changed_cb,
                                     self);
}

void
mpd_display_device_restore_brightness (MpdDisplayDevice     *self,
                                       MpdDisplayDeviceMode  mode)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);
  float     brightness;
  gboolean  hw_changed;

  if (MPD_DISPLAY_DEVICE_MODE_AC == mode)
  {
    brightness = mpd_conf_get_brightness_value (priv->conf);
  } else {
    brightness = mpd_conf_get_brightness_value_battery (priv->conf);
  }

  g_signal_handlers_block_by_func (priv->brightness,
                                   _brightness_changed_cb,
                                   self);

  if (!gpm_brightness_xrandr_set (priv->brightness, brightness * 100, &hw_changed))
  {
    g_warning ("%s : Setting brightness failed", G_STRLOC);
  }

  g_signal_handlers_unblock_by_func (priv->brightness,
                                     _brightness_changed_cb,
                                     self);
}

void
mpd_display_device_increase_brightness (MpdDisplayDevice      *self,
                                        MpdDisplayDeviceMode   mode)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);
  gboolean  hw_changed;

  if (gpm_brightness_xrandr_up (priv->brightness, &hw_changed))
  {
    float brightness = mpd_display_device_get_brightness (self);
    update_stored_brightness (self, brightness, mode);
  } else {
    g_warning ("%s : Increasing brightness failed", G_STRLOC);
  }
}

void
mpd_display_device_decrease_brightness (MpdDisplayDevice      *self,
                                        MpdDisplayDeviceMode   mode)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);
  gboolean hw_changed;

  if (gpm_brightness_xrandr_down (priv->brightness, &hw_changed))
  {
    float brightness = mpd_display_device_get_brightness (self);
    update_stored_brightness (self, brightness, mode);
  } else {
    g_warning ("%s : Decreasing brightness failed", G_STRLOC);
  }
}

