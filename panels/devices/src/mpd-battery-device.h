
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

#ifndef _MPD_BATTERY_DEVICE
#define _MPD_BATTERY_DEVICE

#include <glib-object.h>

G_BEGIN_DECLS

#define mpd_TYPE_BATTERY_DEVICE mpd_battery_device_get_type()

#define mpd_BATTERY_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), mpd_TYPE_BATTERY_DEVICE, MpdBatteryDevice))

#define mpd_BATTERY_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), mpd_TYPE_BATTERY_DEVICE, MpdBatteryDeviceClass))

#define mpd_IS_BATTERY_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), mpd_TYPE_BATTERY_DEVICE))

#define mpd_IS_BATTERY_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), mpd_TYPE_BATTERY_DEVICE))

#define mpd_BATTERY_DEVICE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), mpd_TYPE_BATTERY_DEVICE, MpdBatteryDeviceClass))

typedef struct
{
  GObject parent;
} MpdBatteryDevice;

typedef struct
{
  GObjectClass parent;
} MpdBatteryDeviceClass;

GType
mpd_battery_device_get_type (void);

MpdBatteryDevice *
mpd_battery_device_new (void);

G_END_DECLS

#endif /* _MPD_BATTERY_DEVICE */

