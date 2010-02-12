
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

#ifndef MPD_LID_DEVICE_H
#define MPD_LID_DEVICE_H

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MPD_TYPE_LID_DEVICE mpd_lid_device_get_type()

#define MPD_LID_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_LID_DEVICE, MpdLidDevice))

#define MPD_LID_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_LID_DEVICE, MpdLidDeviceClass))

#define MPD_IS_LID_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_LID_DEVICE))

#define MPD_IS_LID_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_LID_DEVICE))

#define MPD_LID_DEVICE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_LID_DEVICE, MpdLidDeviceClass))

typedef struct
{
  GObject parent;
} MpdLidDevice;

typedef struct
{
  GObjectClass parent;
} MpdLidDeviceClass;

GType
mpd_lid_device_get_type (void);

MpdLidDevice *
mpd_lid_device_new (void);

bool
mpd_lid_device_get_closed (MpdLidDevice *self);

G_END_DECLS

#endif /* MPD_LID_DEVICE_H */

