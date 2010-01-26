
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

#define I_KNOW_THE_DEVICEKIT_POWER_API_IS_SUBJECT_TO_CHANGE

#include <devkit-power-gobject/devicekit-power.h>
#include "mpd-battery-device.h"
#include "config.h"

G_DEFINE_TYPE (MpdBatteryDevice, mpd_battery_device, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), mpd_TYPE_BATTERY_DEVICE, MpdBatteryDevicePrivate))

typedef struct
{
  DkpClient *client;
  DkpDevice *device;
} MpdBatteryDevicePrivate;

static GObject *
_constructor (GType                  type,
              guint                  n_properties,
              GObjectConstructParam *properties)
{
  MpdBatteryDevice *self = (MpdBatteryDevice *)
                              G_OBJECT_CLASS (mpd_battery_device_parent_class)
                                ->constructor (type, n_properties, properties);
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (self);
  GPtrArray *devices;
  GError    *error = NULL;
  guint      i;

  g_return_val_if_fail (priv->client, NULL);

  /* Look up battery device. */

  devices = dkp_client_enumerate_devices (priv->client, &error);
  if (error)
  {
    g_critical ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
    g_object_unref (self);
    return NULL;
  }

  for (i = 0; i < devices->len; i++)
  {
    DkpDevice *device = g_ptr_array_index (devices, i);
    DkpDeviceType device_type;
    g_object_get (device,
                  "type", &device_type,
                  NULL);

    if (DKP_DEVICE_TYPE_BATTERY == device_type)
      priv->device = g_object_ref (device);
  }

  g_ptr_array_unref (devices);

  /* Hook up battery device. */

  g_debug ("%s() %s", __FUNCTION__, dkp_device_get_object_path (priv->device));

  // todo

  return (GObject *) self;
}

static void
_get_property (GObject    *object,
               guint       property_id,
               GValue     *value,
               GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_set_property (GObject      *object,
               guint         property_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (object);

  if (priv->client)
  {
    g_object_unref (priv->client);
    priv->client = NULL;
  }

  G_OBJECT_CLASS (mpd_battery_device_parent_class)->dispose (object);
}

static void
mpd_battery_device_class_init (MpdBatteryDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdBatteryDevicePrivate));

  object_class->constructor = _constructor;
  object_class->dispose = _dispose;
  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
}

static void
mpd_battery_device_init (MpdBatteryDevice *self)
{
  MpdBatteryDevicePrivate *priv = GET_PRIVATE (self);

  priv->client = dkp_client_new ();
}

MpdBatteryDevice *
mpd_battery_device_new (void)
{
  return g_object_new (mpd_TYPE_BATTERY_DEVICE, NULL);
}


