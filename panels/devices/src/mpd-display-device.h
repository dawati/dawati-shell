
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

#ifndef MPD_DISPLAY_DEVICE_H
#define MPD_DISPLAY_DEVICE_H

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MPD_TYPE_DISPLAY_DEVICE mpd_display_device_get_type()

#define MPD_DISPLAY_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_DISPLAY_DEVICE, MpdDisplayDevice))

#define MPD_DISPLAY_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_DISPLAY_DEVICE, MpdDisplayDeviceClass))

#define MPD_IS_DISPLAY_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_DISPLAY_DEVICE))

#define MPD_IS_DISPLAY_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_DISPLAY_DEVICE))

#define MPD_DISPLAY_DEVICE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_DISPLAY_DEVICE, MpdDisplayDeviceClass))

typedef struct
{
  GObject parent;
} MpdDisplayDevice;

typedef struct
{
  GObjectClass parent;
} MpdDisplayDeviceClass;

GType
mpd_display_device_get_type (void);

MpdDisplayDevice *
mpd_display_device_new (void);

bool
mpd_display_device_is_enabled (MpdDisplayDevice  *self);

void
mpd_display_device_set_enabled (MpdDisplayDevice   *self,
                                bool                enabled);

float
mpd_display_device_get_brightness (MpdDisplayDevice  *self);

void
mpd_display_device_set_brightness (MpdDisplayDevice  *self,
                                   float              brightness);

void
mpd_display_device_increase_brightness (MpdDisplayDevice *self);

void
mpd_display_device_decrease_brightness (MpdDisplayDevice *self);

G_END_DECLS

#endif /* MPD_DISPLAY_DEVICE_H */

