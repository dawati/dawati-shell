/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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


#ifndef _PENGE_TWITTER_TILE
#define _PENGE_TWITTER_TILE

#include <glib-object.h>
#include "penge-people-tile.h"

G_BEGIN_DECLS

#define PENGE_TYPE_TWITTER_TILE penge_twitter_tile_get_type()

#define PENGE_TWITTER_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_TWITTER_TILE, PengeTwitterTile))

#define PENGE_TWITTER_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_TWITTER_TILE, PengeTwitterTileClass))

#define PENGE_IS_TWITTER_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_TWITTER_TILE))

#define PENGE_IS_TWITTER_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_TWITTER_TILE))

#define PENGE_TWITTER_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_TWITTER_TILE, PengeTwitterTileClass))

typedef struct {
  PengePeopleTile parent;
} PengeTwitterTile;

typedef struct {
  PengePeopleTileClass parent_class;
} PengeTwitterTileClass;

GType penge_twitter_tile_get_type (void);

G_END_DECLS

#endif /* _PENGE_TWITTER_TILE */

