/*
 * Copyright (C) 2009 Intel Corporation.
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
 */

#ifndef _PENGE_EVERYTHING_PANE
#define _PENGE_EVERYTHING_PANE

#include <glib-object.h>
#include <clutter/clutter.h>
#include "penge-block-container.h"

G_BEGIN_DECLS

#define PENGE_TYPE_EVERYTHING_PANE penge_everything_pane_get_type()

#define PENGE_EVERYTHING_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_EVERYTHING_PANE, PengeEverythingPane))

#define PENGE_EVERYTHING_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_EVERYTHING_PANE, PengeEverythingPaneClass))

#define PENGE_IS_EVERYTHING_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_EVERYTHING_PANE))

#define PENGE_IS_EVERYTHING_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_EVERYTHING_PANE))

#define PENGE_EVERYTHING_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_EVERYTHING_PANE, PengeEverythingPaneClass))

typedef struct _PengeEverythingPanePrivate PengeEverythingPanePrivate;

typedef struct {
  PengeBlockContainer parent;
  PengeEverythingPanePrivate *priv;
} PengeEverythingPane;

typedef struct {
  PengeBlockContainerClass parent_class;
} PengeEverythingPaneClass;

GType penge_everything_pane_get_type (void);

G_END_DECLS

#endif /* _PENGE_EVERYTHING_PANE */

