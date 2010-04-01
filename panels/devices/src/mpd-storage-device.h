
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

#ifndef MPD_STORAGE_DEVICE_H
#define MPD_STORAGE_DEVICE_H

#include <stdbool.h>
#include <stdint.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MPD_TYPE_STORAGE_DEVICE mpd_storage_device_get_type()

#define MPD_STORAGE_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_STORAGE_DEVICE, MpdStorageDevice))

#define MPD_STORAGE_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_STORAGE_DEVICE, MpdStorageDeviceClass))

#define MPD_IS_STORAGE_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_STORAGE_DEVICE))

#define MPD_IS_STORAGE_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_STORAGE_DEVICE))

#define MPD_STORAGE_DEVICE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_STORAGE_DEVICE, MpdStorageDeviceClass))

typedef struct
{
  GObject parent;
} MpdStorageDevice;

typedef struct
{
  GObjectClass parent;
} MpdStorageDeviceClass;

enum
{
  MPD_STORAGE_DEVICE_IMPORT_ERROR_STILL_INDEXING,
  MPD_STORAGE_DEVICE_IMPORT_ERROR_NO_MEDIA,
  MPD_STORAGE_DEVICE_IMPORT_ERROR_INSUFICCIENT_DISK_SPACE
};

GType
mpd_storage_device_get_type (void);

MpdStorageDevice *
mpd_storage_device_new (char const *path);

uint64_t
mpd_storage_device_get_size (MpdStorageDevice *self);

uint64_t
mpd_storage_device_get_available_size (MpdStorageDevice *self);

char const *
mpd_storage_device_get_path (MpdStorageDevice *self);

#if 0 /* Volume crawling code etc. */

char const *
mpd_storage_device_get_label (MpdStorageDevice *self);

char const *
mpd_storage_device_get_model (MpdStorageDevice *self);

char const *
mpd_storage_device_get_vendor (MpdStorageDevice *self);

void
mpd_storage_device_has_media_async (MpdStorageDevice *self);

bool
mpd_storage_device_import_async (MpdStorageDevice  *self,
                                 GError           **error);

bool
mpd_storage_device_stop_import (MpdStorageDevice *self);

#endif

G_END_DECLS

#endif /* MPD_STORAGE_DEVICE_H */

