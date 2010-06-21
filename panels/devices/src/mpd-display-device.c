
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

#include <gconf/gconf-client.h>

#include <gpm/gpm-brightness-xrandr.h>

#include "mpd-display-device.h"
#include "mpd-gobject.h"

#define MPD_GCONF_DIR "/desktop/meego/panel-devices"
#define MPD_DISPLAY_BRIGHTNESS_ENABLED "brightness_enabled"
#define MPD_DISPLAY_BRIGHTNESS_VALUE "brightness_value"

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
  GConfClient         *client;
  unsigned int         client_connection_id;
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
_gconf_notify_cb (GConfClient       *client,
                  unsigned int       connection_id,
                  GConfEntry        *entry,
                  MpdDisplayDevice  *self)
{
  char const  *key;
  GConfValue  *value;

  g_debug ("%s()", __FUNCTION__);

  key = gconf_entry_get_key (entry);
  value = gconf_entry_get_value (entry);

  if (0 == g_strcmp0 (key, MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_ENABLED))
  {
    g_object_notify (G_OBJECT (self), "enabled");
  }

  if (0 == g_strcmp0 (key, MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_VALUE))
  {
    g_object_notify (G_OBJECT (self), "brightness");
  }
}

static void
_get_property (GObject      *object,
               unsigned int  property_id,
               GValue       *value,
               GParamSpec   *pspec)
{
  GError *error = NULL;

  switch (property_id) {
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

  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
}

static void
_set_property (GObject      *object,
               unsigned int  property_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  GError *error = NULL;

  switch (property_id) {
  case PROP_ENABLED:
    mpd_display_device_set_enabled (MPD_DISPLAY_DEVICE (object),
                                    g_value_get_boolean (value));
    break;
  case PROP_BRIGHTNESS:
    mpd_display_device_set_brightness (MPD_DISPLAY_DEVICE (object),
                                       g_value_get_float (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }

  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
}

static void
_dispose (GObject *object)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (object);

  if (priv->client_connection_id)
  {
    gconf_client_notify_remove (priv->client, priv->client_connection_id);
    priv->client_connection_id = 0;
  }

  mpd_gobject_detach (object, (GObject **) &priv->client);

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

  param_flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_ENABLED,
                                   g_param_spec_boolean ("enabled",
                                                         "Enabled",
                                                         "If display brightness is enabled",
                                                         false,
                                                         param_flags));

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
  GError  *error = NULL;

  priv->brightness = gpm_brightness_xrandr_new ();

  priv->client = gconf_client_get_default ();
  gconf_client_add_dir (priv->client,
                        MPD_GCONF_DIR,
                        GCONF_CLIENT_PRELOAD_ONELEVEL,
                        &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  } else
  {
    priv->client_connection_id = gconf_client_notify_add (
                                      priv->client,
                                      MPD_GCONF_DIR,
                                      (GConfClientNotifyFunc) _gconf_notify_cb,
                                      self,
                                      NULL,
                                      &error);
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }
  }

  if (mpd_display_device_is_enabled (self))
  {
    /* Reset stored brightness. */
    float brightness = gconf_client_get_float (
                                  priv->client,
                                  MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_VALUE,
                                  &error);
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    } else
    {
      gboolean hw_changed;
      brightness = CLAMP (brightness, 0.0, 1.0);
      gpm_brightness_xrandr_set (priv->brightness,
                                 brightness * 100,
                                 &hw_changed);
    }
  }

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
  GError  *error = NULL;
  bool     ret;

  g_return_val_if_fail (MPD_IS_DISPLAY_DEVICE (self), false);

  ret = gconf_client_get_bool (priv->client,
                               MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_ENABLED,
                               &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  return ret;
}

void
mpd_display_device_set_enabled (MpdDisplayDevice   *self,
                                bool                enabled)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);
  GError  *error = NULL;

  g_return_if_fail (MPD_IS_DISPLAY_DEVICE (self));

  gconf_client_set_bool (priv->client,
                         MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_ENABLED,
                         enabled,
                         &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
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

void
mpd_display_device_set_brightness (MpdDisplayDevice  *self,
                                   float              brightness)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);
  gboolean hw_changed;

  g_return_if_fail (MPD_IS_DISPLAY_DEVICE (self));

  brightness = CLAMP (brightness, 0.0, 1.0);

  if (gpm_brightness_xrandr_set (priv->brightness, brightness * 100, &hw_changed))
  {
    /* Also update gconf */
    GError *error = NULL;
    gconf_client_set_float (priv->client,
                            MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_VALUE,
                            brightness,
                            &error);
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }
  } else
  {
    g_warning ("%s : Setting brightness failed", G_STRLOC);
  }
}

