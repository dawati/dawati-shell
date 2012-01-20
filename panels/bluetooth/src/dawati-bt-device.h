/*
 * Copyright (c) 2012 Intel Corp.
 *
 * Author: Jussi Kukkonen <jussi.kukkonen@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _DAWATI_BT_DEVICE
#define _DAWATI_BT_DEVICE

#include <mx/mx.h>

G_BEGIN_DECLS

#define DAWATI_TYPE_BT_DEVICE dawati_bt_device_get_type()

#define DAWATI_BT_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DAWATI_TYPE_BT_DEVICE, DawatiBtDevice))

#define DAWATI_BT_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DAWATI_TYPE_BT_DEVICE, DawatiBtDeviceClass))

#define DAWATI_IS_BT_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DAWATI_TYPE_BT_DEVICE))

#define DAWATI_IS_BT_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DAWATI_TYPE_BT_DEVICE))

#define DAWATI_BT_DEVICE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DAWATI_TYPE_BT_DEVICE, DawatiBtDeviceClass))

typedef struct {
  MxBoxLayout parent;
} DawatiBtDevice;

typedef struct {
  MxBoxLayoutClass parent_class;
  void (*disconnect) (DawatiBtDevice  *device);
} DawatiBtDeviceClass;

GType dawati_bt_device_get_type (void);
ClutterActor* dawati_bt_device_new (const char *name, const char *device_path);

G_END_DECLS

#endif /* _DAWATI_BT_DEVICE */
