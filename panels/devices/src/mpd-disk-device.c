
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

#include <errno.h>
#include <string.h>
#include <sys/statvfs.h>
#include "mpd-disk-device.h"

G_DEFINE_TYPE (MpdDiskDevice, mpd_disk_device, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_DISK_DEVICE, MpdDiskDevicePrivate))

enum
{
  PROP_0,

  PROP_SIZE,
  PROP_AVAILABLE_SIZE
};

typedef struct
{
  guint   update_timeout_id;
  guint64 size;
  guint64 available_size;
} MpdDiskDevicePrivate;

static void
mpd_disk_device_set_size (MpdDiskDevice *self,
                          guint64        size);

static void
mpd_disk_device_set_available_size (MpdDiskDevice *self,
                                    guint64        available_size);

static void
update (MpdDiskDevice *self)
{
  struct statvfs fsd = { 0, };

  if (0 == statvfs ("/", &fsd))
  {
    mpd_disk_device_set_size (self, (guint64) fsd.f_blocks * fsd.f_frsize);
    mpd_disk_device_set_available_size (self, (guint64) fsd.f_bavail * fsd.f_frsize);
  } else {
    g_warning ("%s : %s", G_STRLOC, strerror (errno));
  }
}

static gboolean
_update_timeout_cb (MpdDiskDevice *self)
{
  update (self);
  return TRUE;
}

static void
_get_property (GObject    *object,
               guint       property_id,
               GValue     *value,
               GParamSpec *pspec)
{
  switch (property_id) {
  case PROP_SIZE:
    g_value_set_uint64 (value,
                        mpd_disk_device_get_size (MPD_DISK_DEVICE (object)));
    break;
  case PROP_AVAILABLE_SIZE:
    g_value_set_uint64 (value,
                        mpd_disk_device_get_available_size
                          (MPD_DISK_DEVICE (object)));
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
  case PROP_SIZE:
    mpd_disk_device_set_size (MPD_DISK_DEVICE (object),
                              g_value_get_uint64 (value));
    break;
  case PROP_AVAILABLE_SIZE:
    mpd_disk_device_set_available_size (MPD_DISK_DEVICE (object),
                                        g_value_get_uint64 (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  MpdDiskDevicePrivate *priv = GET_PRIVATE (object);

  if (priv->update_timeout_id)
  {
    g_source_remove (priv->update_timeout_id);
    priv->update_timeout_id = 0;
  }

  G_OBJECT_CLASS (mpd_disk_device_parent_class)->dispose (object);
}

static void
mpd_disk_device_class_init (MpdDiskDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamFlags   param_flags;

  g_type_class_add_private (klass, sizeof (MpdDiskDevicePrivate));

  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
  object_class->dispose = _dispose;

  /* Properties */

  param_flags = G_PARAM_READABLE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_SIZE,
                                   g_param_spec_uint64 ("size",
                                                        "Size",
                                                        "Disk size",
                                                        0, G_MAXUINT64, 0,
                                                        param_flags));
  g_object_class_install_property (object_class,
                                   PROP_AVAILABLE_SIZE,
                                   g_param_spec_uint64 ("available-size",
                                                        "Available size",
                                                        "Available disk size",
                                                        0, G_MAXUINT64, 0,
                                                        param_flags));
}

static void
mpd_disk_device_init (MpdDiskDevice *self)
{
  MpdDiskDevicePrivate *priv = GET_PRIVATE (self);

  update (self);
  priv->update_timeout_id =
          g_timeout_add_seconds (60, (GSourceFunc) _update_timeout_cb, self);

#if 0
  GList *devices;
  GList *iter;

  priv->pool = gdu_pool_new ();
  g_return_if_fail (priv->pool);

  devices = gdu_pool_get_devices (priv->pool);
  for (iter = devices; iter; iter = iter->next)
  {
    GduDevice *device = GDU_DEVICE (iter->data);
    g_debug ("%s", gdu_device_get_device_file (device));
    if (gdu_device_is_mounted (device) &&
        0 == g_strcmp0 ("/", gdu_device_get_mount_path (device)))
    {
      g_debug ("  / size %lld", gdu_device_get_size (device));
    }
  }
  g_list_foreach (devices, (GFunc) g_object_unref, NULL);
  g_list_free (devices);
#endif
}

MpdDiskDevice *
mpd_disk_device_new (void)
{
  return g_object_new (MPD_TYPE_DISK_DEVICE, NULL);
}

guint64
mpd_disk_device_get_size (MpdDiskDevice *self)
{
  MpdDiskDevicePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_DISK_DEVICE (self), 0);

  return priv->size;
}

static void
mpd_disk_device_set_size (MpdDiskDevice *self,
                          guint64        size)
{
  MpdDiskDevicePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_DISK_DEVICE (self));

  if (size != priv->size)
  {
    priv->size = size;
    g_object_notify (G_OBJECT (self), "size");
  }
}

guint64
mpd_disk_device_get_available_size (MpdDiskDevice *self)
{
  MpdDiskDevicePrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_DISK_DEVICE (self), 0);

  return priv->available_size;
}

static void
mpd_disk_device_set_available_size (MpdDiskDevice *self,
                                    guint64        available_size)
{
  MpdDiskDevicePrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_DISK_DEVICE (self));

  if (available_size != priv->available_size)
  {
    priv->available_size = available_size;
    g_object_notify (G_OBJECT (self), "available-size");
  }
}

