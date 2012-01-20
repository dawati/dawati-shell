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

#ifndef _DAWATI_BT_REQUEST
#define _DAWATI_BT_REQUEST

#include <mx/mx.h>

G_BEGIN_DECLS

#define DAWATI_TYPE_BT_REQUEST dawati_bt_request_get_type()

#define DAWATI_BT_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DAWATI_TYPE_BT_REQUEST, DawatiBtRequest))

#define DAWATI_BT_REQUEST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DAWATI_TYPE_BT_REQUEST, DawatiBtRequestClass))

#define DAWATI_IS_BT_REQUEST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DAWATI_TYPE_BT_REQUEST))

#define DAWATI_IS_BT_REQUEST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DAWATI_TYPE_BT_REQUEST))

#define DAWATI_BT_REQUEST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DAWATI_TYPE_BT_REQUEST, DawatiBtRequestClass))

typedef enum {
  DAWATI_BT_REQUEST_TYPE_PIN = 0,
  DAWATI_BT_REQUEST_TYPE_PASSKEY,
  DAWATI_BT_REQUEST_TYPE_CONFIRM,
  DAWATI_BT_REQUEST_TYPE_AUTH,
} DawatiBtRequestType;

typedef enum {
  DAWATI_BT_REQUEST_RESPONSE_NO = 0,
  DAWATI_BT_REQUEST_RESPONSE_YES,
  DAWATI_BT_REQUEST_RESPONSE_ALWAYS,
} DawatiBtResponse;

typedef struct {
  MxBoxLayout parent;
} DawatiBtRequest;

typedef struct {
  MxBoxLayoutClass parent_class;
  void (*response) (DawatiBtRequest *request, DawatiBtRequestType type, const char *data);
} DawatiBtRequestClass;

GType dawati_bt_request_get_type (void);
ClutterActor* dawati_bt_request_new (const char *name, const char *device_path, DawatiBtRequestType type, const char *data);

G_END_DECLS

#endif /* _DAWATI_BT_REQUEST */
