/*
 * Copyright (C) 2010, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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

#ifndef _PENGE_DYNAMIC_BOX
#define _PENGE_DYNAMIC_BOX

#include <mx/mx.h>

G_BEGIN_DECLS

#define PENGE_TYPE_DYNAMIC_BOX penge_dynamic_box_get_type()

#define PENGE_DYNAMIC_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_DYNAMIC_BOX, PengeDynamicBox))

#define PENGE_DYNAMIC_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_DYNAMIC_BOX, PengeDynamicBoxClass))

#define PENGE_IS_DYNAMIC_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_DYNAMIC_BOX))

#define PENGE_IS_DYNAMIC_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_DYNAMIC_BOX))

#define PENGE_DYNAMIC_BOX_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_DYNAMIC_BOX, PengeDynamicBoxClass))

typedef struct _PengeDynamicBoxPrivate PengeDynamicBoxPrivate;

typedef struct {
  MxWidget parent;
  PengeDynamicBoxPrivate *priv;
} PengeDynamicBox;

typedef struct {
  MxWidgetClass parent_class;
} PengeDynamicBoxClass;

GType penge_dynamic_box_get_type (void);

ClutterActor *penge_dynamic_box_new (void);

G_END_DECLS

#endif /* _PENGE_DYNAMIC_BOX */

