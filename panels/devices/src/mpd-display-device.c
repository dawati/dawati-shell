
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

#include "mpd-display-device.h"

#define MPD_GCONF_DIR "/desktop/moblin/panel-devices"
#define MPD_DISPLAY_BRIGHTNESS_ENABLED "brightness_enabled"
#define MPD_DISPLAY_BRIGHTNESS_PERCENTAGE "brightness_percentage"

G_DEFINE_TYPE (MpdDisplayDevice, mpd_display_device, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_DISPLAY_DEVICE, MpdDisplayDevicePrivate))

enum
{
  PROP_0,

  PROP_ENABLED,
  PROP_PERCENTAGE
};

typedef struct
{
  GConfClient   *client;
  unsigned int   client_connection_id;
} MpdDisplayDevicePrivate;

static void
_gconf_notify_cb (GConfClient       *client,
                  unsigned int       connection_id,
                  GConfEntry        *entry,
                  MpdDisplayDevice  *self)
{
  char const  *key;
  GConfValue  *value;
  GError      *error = NULL;

  key = gconf_entry_get_key (entry);
  value = gconf_entry_get_value (entry);

  if (0 == g_strcmp0 (key, MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_ENABLED))
  {
    g_object_notify (G_OBJECT (self), "enabled");
  }

  if (0 == g_strcmp0 (key, MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_PERCENTAGE) &&
      mpd_display_device_is_enabled (self, &error))
  {
    g_object_notify (G_OBJECT (self), "percentage");
  }

  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
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
                          MPD_DISPLAY_DEVICE (object),
                          &error));
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }
    break;
  case PROP_PERCENTAGE:
    g_value_set_int (value,
                     mpd_display_device_get_percentage (
                      MPD_DISPLAY_DEVICE (object),
                      &error));
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }
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
  GError *error = NULL;

  switch (property_id) {
  /* Read-only
  case PROP_ENABLED:
    mpd_display_device_set_enabled (MPD_DISPLAY_DEVICE (object),
                                    g_value_get_boolean (value),
                                    &error);
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }
    break;
  */
  case PROP_PERCENTAGE:
    mpd_display_device_set_percentage (MPD_DISPLAY_DEVICE (object),
                                       g_value_get_int (value),
                                       &error);
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
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

  if (priv->client)
  {
    g_object_unref (priv->client);
    priv->client = NULL;
  }

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

  g_object_class_install_property (object_class,
                                   PROP_PERCENTAGE,
                                   g_param_spec_int ("percentage",
                                                     "Percentage",
                                                     "Display brightess percentage",
                                                     -1, 100, 100,
                                                     param_flags |
                                                     G_PARAM_WRITABLE));

}

static void
mpd_display_device_init (MpdDisplayDevice *self)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);
  bool     have_dir;
  GError  *error = NULL;

  priv->client = gconf_client_get_default ();

  have_dir = gconf_client_dir_exists (priv->client,
                                      MPD_GCONF_DIR,
                                      &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);

  } else if (have_dir)
  {
    gconf_client_add_dir (priv->client,
                          MPD_GCONF_DIR,
                          GCONF_CLIENT_PRELOAD_ONELEVEL,
                          &error);
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
}

MpdDisplayDevice *
mpd_display_device_new (void)
{
  return g_object_new (MPD_TYPE_DISPLAY_DEVICE, NULL);
}

bool
mpd_display_device_is_enabled (MpdDisplayDevice  *self,
                               GError           **error)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_DISPLAY_DEVICE (self), false);

  return gconf_client_get_bool (priv->client,
                                MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_ENABLED,
                                error);
}

int
mpd_display_device_get_percentage (MpdDisplayDevice  *self,
                                   GError           **error)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_DISPLAY_DEVICE (self), -1);

  if (!mpd_display_device_is_enabled (self, error))
  {
    g_warning ("%s : Display brightness is not enabled.", G_STRLOC);
    return -1;
  }

  /* Should fail if the key doesn't exist ... */
  if (!gconf_client_key_is_writable (
                            priv->client,
                            MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_PERCENTAGE,
                            error))
  {
    return -1;
  }

  return gconf_client_get_int (
                            priv->client,
                            MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_PERCENTAGE,
                            error);
}

void
mpd_display_device_set_percentage (MpdDisplayDevice  *self,
                                   unsigned int       percentage,
                                   GError           **error)
{
  MpdDisplayDevicePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_DISPLAY_DEVICE (self));

  if (!mpd_display_device_is_enabled (self, error))
  {
    g_warning ("%s : Display brightness is not enabled.", G_STRLOC);
    return;
  }

  /* Should fail if the key doesn't exist ... */
  if (!gconf_client_key_is_writable (
                        priv->client,
                        MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_PERCENTAGE,
                        error))
  {
    g_warning ("%s : GConf key '%s' not writable",
               G_STRLOC,
               MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_PERCENTAGE);
    return;
  }

  gconf_client_set_int (priv->client,
                        MPD_GCONF_DIR"/"MPD_DISPLAY_BRIGHTNESS_PERCENTAGE,
                        percentage,
                        error);

  /* Notification is done through the gconf monitor. */
}

