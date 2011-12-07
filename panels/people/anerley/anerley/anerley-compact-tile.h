/*
 * Anerley - people feeds and widgets
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
 *
 */

#ifndef _ANERLEY_COMPACT_TILE
#define _ANERLEY_COMPACT_TILE

#include <mx/mx.h>
#include <anerley/anerley-item.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_COMPACT_TILE anerley_compact_tile_get_type()

#define ANERLEY_COMPACT_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANERLEY_TYPE_COMPACT_TILE, AnerleyCompactTile))

#define ANERLEY_COMPACT_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), ANERLEY_TYPE_COMPACT_TILE, AnerleyCompactTileClass))

#define ANERLEY_IS_COMPACT_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANERLEY_TYPE_COMPACT_TILE))

#define ANERLEY_IS_COMPACT_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANERLEY_TYPE_COMPACT_TILE))

#define ANERLEY_COMPACT_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANERLEY_TYPE_COMPACT_TILE, AnerleyCompactTileClass))

typedef struct {
  MxWidget parent;
} AnerleyCompactTile;

typedef struct {
  MxWidgetClass parent_class;
} AnerleyCompactTileClass;

GType anerley_compact_tile_get_type (void);
AnerleyItem *anerley_compact_tile_get_item (AnerleyCompactTile *tile);
G_END_DECLS

#endif /* _ANERLEY_COMPACT_TILE */
