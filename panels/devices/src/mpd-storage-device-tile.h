
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

#ifndef MPD_STORAGE_DEVICE_TILE_H
#define MPD_STORAGE_DEVICE_TILE_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MPD_TYPE_STORAGE_DEVICE_TILE mpd_storage_device_tile_get_type()

#define MPD_STORAGE_DEVICE_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_STORAGE_DEVICE_TILE, MpdStorageDeviceTile))

#define MPD_STORAGE_DEVICE_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_STORAGE_DEVICE_TILE, MpdStorageDeviceTileClass))

#define MPD_IS_STORAGE_DEVICE_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_STORAGE_DEVICE_TILE))

#define MPD_IS_STORAGE_DEVICE_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_STORAGE_DEVICE_TILE))

#define MPD_STORAGE_DEVICE_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_STORAGE_DEVICE_TILE, MpdStorageDeviceTileClass))

typedef struct
{
  MxBoxLayout parent;
} MpdStorageDeviceTile;

typedef struct
{
  MxBoxLayoutClass parent;
} MpdStorageDeviceTileClass;

GType
mpd_storage_device_tile_get_type (void);

ClutterActor *
mpd_storage_device_tile_new (char const *mount_point,
                             char const *icon_file);

char const *
mpd_storage_device_tile_get_icon_file (MpdStorageDeviceTile *self);

char const *
mpd_storage_device_tile_get_mount_point (MpdStorageDeviceTile *self);

G_END_DECLS

#endif /* MPD_STORAGE_DEVICE_TILE_H */

