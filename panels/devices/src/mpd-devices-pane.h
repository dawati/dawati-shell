
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

#ifndef MPD_DEVICES_PANE_H
#define MPD_DEVICES_PANE_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MPD_TYPE_DEVICES_PANE mpd_devices_pane_get_type()

#define MPD_DEVICES_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_DEVICES_PANE, MpdDevicesPane))

#define MPD_DEVICES_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_DEVICES_PANE, MpdDevicesPaneClass))

#define MPD_IS_DEVICES_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_DEVICES_PANE))

#define MPD_IS_DEVICES_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_DEVICES_PANE))

#define MPD_DEVICES_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_DEVICES_PANE, MpdDevicesPaneClass))

typedef struct
{
  MxTable parent;
} MpdDevicesPane;

typedef struct
{
  MxTableClass parent_class;
} MpdDevicesPaneClass;

GType
mpd_devices_pane_get_type (void);

ClutterActor *
mpd_devices_pane_new (void);

G_END_DECLS

#endif /* MPD_DEVICES_PANE_H */

