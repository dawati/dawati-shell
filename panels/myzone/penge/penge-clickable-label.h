/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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
 *
 */

#ifndef _PENGE_CLICKABLE_LABEL
#define _PENGE_CLICKABLE_LABEL

#include <mx/mx.h>

G_BEGIN_DECLS

#define PENGE_TYPE_CLICKABLE_LABEL penge_clickable_label_get_type()

#define PENGE_CLICKABLE_LABEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_CLICKABLE_LABEL, PengeClickableLabel))

#define PENGE_CLICKABLE_LABEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_CLICKABLE_LABEL, PengeClickableLabelClass))

#define PENGE_IS_CLICKABLE_LABEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_CLICKABLE_LABEL))

#define PENGE_IS_CLICKABLE_LABEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_CLICKABLE_LABEL))

#define PENGE_CLICKABLE_LABEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_CLICKABLE_LABEL, PengeClickableLabelClass))

typedef struct _PengeClickableLabelPrivate PengeClickableLabelPrivate;

typedef struct {
  MxLabel parent;
  PengeClickableLabelPrivate *priv;
} PengeClickableLabel;

typedef struct {
  MxLabelClass parent_class;
} PengeClickableLabelClass;

GType penge_clickable_label_get_type (void);

ClutterActor *penge_clickable_label_new (const gchar *text);

G_END_DECLS

#endif /* _PENGE_CLICKABLE_LABEL */
