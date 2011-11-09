/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Srinivasa Ragavan <srini@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MNP_BUTTON_ITEM_H
#define MNP_BUTTON_ITEM_H

#include <glib.h>
#include <glib-object.h>
#include <mx/mx.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MNP_TYPE_BUTTON_ITEM_TYPE             (mnp_button_item_get_type ())
#define MNP_BUTTON_ITEM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNP_TYPE_BUTTON_ITEM_TYPE, MnpButtonItem))
#define MNP_BUTTON_ITEM_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), MNP_TYPE_BUTTON_ITEM_TYPE, MnpButtonItemClass))
#define TNY_IS_DETAIL_TYPE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNP_TYPE_BUTTON_ITEM_TYPE))
#define TNY_IS_DETAIL_TYPE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), MNP_TYPE_BUTTON_ITEM_TYPE))
#define MNP_BUTTON_ITEM_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), MNP_TYPE_BUTTON_ITEM_TYPE, MnpButtonItemClass))

typedef struct _MnpButtonItem MnpButtonItem;
typedef struct _MnpButtonItemClass MnpButtonItemClass;

typedef struct _MnpButtonItemPriv MnpButtonItemPriv;

typedef void (*completion_done) (gpointer, const char *);
struct _MnpButtonItem
{
	GObject parent;

	MnpButtonItemPriv *priv;
};

struct _MnpButtonItemClass 
{
	GObjectClass parent;
};

GType mnp_button_item_get_type (void);
MnpButtonItem * mnp_button_item_new (gpointer user_data, completion_done func);

G_END_DECLS

#endif

