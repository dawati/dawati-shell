/*
 * Anerley - people feeds and widgets
 * Copyright (C) 2009, Intel Corporation.
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
 *
 */


#include <mx/mx.h>
#include <anerley/anerley-feed.h>

#ifndef _ANERLEY_SIMPLE_GRID_VIEW_H
#define _ANERLEY_SIMPLE_GRID_VIEW_H

#include <glib-object.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_SIMPLE_GRID_VIEW anerley_simple_grid_view_get_type()

#define ANERLEY_SIMPLE_GRID_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  ANERLEY_TYPE_SIMPLE_GRID_VIEW, AnerleySimpleGridView))

#define ANERLEY_SIMPLE_GRID_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  ANERLEY_TYPE_SIMPLE_GRID_VIEW, AnerleySimpleGridViewClass))

#define ANERLEY_IS_SIMPLE_GRID_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  ANERLEY_TYPE_SIMPLE_GRID_VIEW))

#define ANERLEY_IS_SIMPLE_GRID_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  ANERLEY_TYPE_SIMPLE_GRID_VIEW))

#define ANERLEY_SIMPLE_GRID_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  ANERLEY_TYPE_SIMPLE_GRID_VIEW, AnerleySimpleGridViewClass))

typedef struct {
  MxGrid parent;
} AnerleySimpleGridView;

typedef struct {
  MxGridClass parent_class;
} AnerleySimpleGridViewClass;

GType anerley_simple_grid_view_get_type (void);

ClutterActor *anerley_simple_grid_view_new (AnerleyFeed *feed);

G_END_DECLS

#endif /* _ANERLEY_SIMPLE_GRID_VIEW_H */
