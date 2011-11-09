/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
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


#ifndef _PENGE_PEOPLE_TILE
#define _PENGE_PEOPLE_TILE

#include <glib-object.h>
#include <mx/mx.h>
#include <penge/penge-interesting-tile.h>
#include <libsocialweb-client/sw-item.h>

G_BEGIN_DECLS

#define PENGE_TYPE_PEOPLE_TILE penge_people_tile_get_type()

#define PENGE_PEOPLE_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_PEOPLE_TILE, PengePeopleTile))

#define PENGE_PEOPLE_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_PEOPLE_TILE, PengePeopleTileClass))

#define PENGE_IS_PEOPLE_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_PEOPLE_TILE))

#define PENGE_IS_PEOPLE_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_PEOPLE_TILE))

#define PENGE_PEOPLE_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_PEOPLE_TILE, PengePeopleTileClass))

typedef struct _PengePeopleTilePrivate PengePeopleTilePrivate;

typedef struct {
  PengeInterestingTile parent;
  PengePeopleTilePrivate *priv;
} PengePeopleTile;

typedef struct {
  PengeInterestingTileClass parent_class;
} PengePeopleTileClass;

GType penge_people_tile_get_type (void);
void penge_people_tile_activate (PengePeopleTile *tile,
                                 SwItem          *item);
void penge_people_tile_refresh (PengePeopleTile *tile);

G_END_DECLS

#endif /* _PENGE_PEOPLE_TILE */

