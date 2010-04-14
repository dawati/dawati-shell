
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

#ifndef MPD_BATTERY_ICON_H
#define MPD_BATTERY_ICON_H

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MPD_TYPE_BATTERY_ICON mpd_battery_icon_get_type()

#define MPD_BATTERY_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_BATTERY_ICON, MpdBatteryIcon))

#define MPD_BATTERY_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_BATTERY_ICON, MpdBatteryIconClass))

#define MPD_IS_BATTERY_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_BATTERY_ICON))

#define MPD_IS_BATTERY_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_BATTERY_ICON))

#define MPD_BATTERY_ICON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_BATTERY_ICON, MpdBatteryIconClass))

typedef struct
{
  ClutterTexture parent;
} MpdBatteryIcon;

typedef struct
{
  ClutterTextureClass parent;
} MpdBatteryIconClass;

GType
mpd_battery_icon_get_type (void);

ClutterActor *
mpd_battery_icon_new (void);

unsigned int
mpd_battery_icon_get_fps (MpdBatteryIcon *self);

void
mpd_battery_icon_set_fps (MpdBatteryIcon *self,
                          unsigned int    fps);

void
mpd_battery_icon_animate (MpdBatteryIcon  *self,
                          GList           *frames);

GList *
mpd_battery_icon_load_frames_from_dir (char const  *path,
                                       GError     **error);

G_END_DECLS

#endif /* MPD_BATTERY_ICON_H */

