
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

#include <gdu/gdu.h>
#include "mpd-disk-device.h"

G_DEFINE_TYPE (MpdDiskDevice, mpd_disk_device, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_DISK_DEVICE, MpdDiskDevicePrivate))

typedef struct
{
  GduPool *pool;
} MpdDiskDevicePrivate;

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
  MpdDiskDevicePrivate *priv = GET_PRIVATE (object);

  if (priv->pool)
  {
    g_object_unref (priv->pool);
    priv->pool = NULL;
  }

  G_OBJECT_CLASS (mpd_disk_device_parent_class)->dispose (object);
}

static void
mpd_disk_device_class_init (MpdDiskDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdDiskDevicePrivate));

  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
  object_class->dispose = _dispose;
}

static void
mpd_disk_device_init (MpdDiskDevice *self)
{
  MpdDiskDevicePrivate *priv = GET_PRIVATE (self);
  GList *devices;
  GList *iter;

  priv->pool = gdu_pool_new ();
  g_return_if_fail (priv->pool);

  devices = gdu_pool_get_devices (priv->pool);
  for (iter = devices; iter; iter = iter->next)
  {
    GduDevice *device = GDU_DEVICE (iter->data);
    g_debug ("%s", gdu_device_get_device_file (device));
  }
  g_list_foreach (devices, (GFunc) g_object_unref, NULL);
  g_list_free (devices);
}

MpdDiskDevice *
mpd_disk_device_new (void)
{
  return g_object_new (MPD_TYPE_DISK_DEVICE, NULL);
}

