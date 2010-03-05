/*
 * moblin-app-installer -- Moblin garage client application.
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 */

#ifndef PENGE_COUNT_PANE_H
#define PENGE_COUNT_PANE_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define PENGE_TYPE_COUNT_PANE penge_count_pane_get_type()

#define PENGE_COUNT_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_COUNT_PANE, PengeCountPane))

#define PENGE_COUNT_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_COUNT_PANE, PengeCountPaneClass))

#define MAI_IS_VOLUME_CONTROL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_COUNT_PANE))

#define MAI_IS_VOLUME_CONTROL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_COUNT_PANE))

#define PENGE_COUNT_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_COUNT_PANE, PengeCountPaneClass))

typedef struct _PengeCountPanePrivate PengeCountPanePrivate;

typedef struct
{
  MxButton parent;
  PengeCountPanePrivate *priv;
} PengeCountPane;

typedef struct
{
  MxScrollBarClass parent_class;

} PengeCountPaneClass;

GType         penge_count_pane_get_type (void);
MxWidget     *penge_count_pane_new (void);

G_END_DECLS

#endif /* PENGE_COUNT_PANE_H */

