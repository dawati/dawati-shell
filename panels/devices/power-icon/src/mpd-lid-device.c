
/*
 * Copyright (C) 2009-2010, Intel Corporation.
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

#include <devkit-power-gobject/devicekit-power.h>
#include "mpd-lid-device.h"
#include "config.h"

static void
mpd_lid_device_set_closed (MpdLidDevice *self,
                           bool          closed);

G_DEFINE_TYPE (MpdLidDevice, mpd_lid_device, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_LID_DEVICE, MpdLidDevicePrivate))

enum
{
  PROP_0,

  PROP_CLOSED /* Read-only. */
};

typedef struct
{
  DkpClient *client;
  bool       closed;
} MpdLidDevicePrivate;

static void
_client_device_changed_cb (DkpClient    *client,
                           DkpDevice    *device,
                           MpdLidDevice *self)
{
  bool closed;

	g_object_get (client,
                "lid-is-closed", &closed,
                NULL);

  mpd_lid_device_set_closed (self, closed);
}

static void
_get_property (GObject    *object,
               guint       property_id,
               GValue     *value,
               GParamSpec *pspec)
{
  switch (property_id) {
  case PROP_CLOSED:
    g_value_set_boolean (value,
                         mpd_lid_device_get_closed (MPD_LID_DEVICE (object)));
    break;
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
  /* PROP_CLOSED is r/o. */
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  MpdLidDevicePrivate *priv = GET_PRIVATE (object);

  if (priv->client)
  {
    g_object_unref (priv->client);
    priv->client = NULL;
  }

  G_OBJECT_CLASS (mpd_lid_device_parent_class)->dispose (object);
}

static void
mpd_lid_device_class_init (MpdLidDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamFlags   param_flags;

  g_type_class_add_private (klass, sizeof (MpdLidDevicePrivate));

  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
  object_class->dispose = _dispose;

  /* Properties */

  param_flags = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_CLOSED,
                                   g_param_spec_boolean ("closed",
                                                         "Closed",
                                                         "Lid closed",
                                                         false,
                                                         param_flags));
}

static void
mpd_lid_device_init (MpdLidDevice *self)
{
  MpdLidDevicePrivate *priv = GET_PRIVATE (self);

  priv->client = dkp_client_new ();
  g_signal_connect (priv->client, "device-changed",
                    G_CALLBACK (_client_device_changed_cb), self);
	g_object_get (priv->client,
                "lid-is-closed", &priv->closed,
                NULL);
}

MpdLidDevice *
mpd_lid_device_new (void)
{
  return g_object_new (MPD_TYPE_LID_DEVICE, NULL);
}

bool
mpd_lid_device_get_closed (MpdLidDevice *self)
{
  MpdLidDevicePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_LID_DEVICE (self), false);

  return priv->closed;
}

static void
mpd_lid_device_set_closed (MpdLidDevice *self,
                           bool          closed)
{
  MpdLidDevicePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_LID_DEVICE (self));

  if (closed != priv->closed)
  {
    priv->closed = closed;
    g_object_notify (G_OBJECT (self), "closed");
  }
}

