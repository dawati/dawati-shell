/*
 *
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
 */

#ifndef _PENGE_BLOCK_LAYOUT
#define _PENGE_BLOCK_LAYOUT

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define PENGE_TYPE_BLOCK_LAYOUT penge_block_layout_get_type()

#define PENGE_BLOCK_LAYOUT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_BLOCK_LAYOUT, PengeBlockLayout))

#define PENGE_BLOCK_LAYOUT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_BLOCK_LAYOUT, PengeBlockLayoutClass))

#define PENGE_IS_BLOCK_LAYOUT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_BLOCK_LAYOUT))

#define PENGE_IS_BLOCK_LAYOUT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_BLOCK_LAYOUT))

#define PENGE_BLOCK_LAYOUT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_BLOCK_LAYOUT, PengeBlockLayoutClass))

typedef struct {
  ClutterLayoutManager parent;
} PengeBlockLayout;

typedef struct {
  ClutterLayoutManagerClass parent_class;
} PengeBlockLayoutClass;

GType penge_block_layout_get_type (void);

ClutterLayoutManager *penge_block_layout_new (void);

void penge_block_layout_set_spacing (PengeBlockLayout *pbl,
                                     gfloat            spacing);
void penge_block_layout_set_min_tile_size (PengeBlockLayout *pbl,
                                           gfloat            width,
                                           gfloat            height);

G_END_DECLS

#endif /* _PENGE_BLOCK_LAYOUT */

