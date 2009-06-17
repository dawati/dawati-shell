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


#ifndef _PENGE_PEOPLE_TILE
#define _PENGE_PEOPLE_TILE

#include <glib-object.h>
#include <nbtk/nbtk.h>
#include <mojito-client/mojito-item.h>

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

typedef struct {
  NbtkTable parent;
} PengePeopleTile;

typedef struct {
  NbtkTableClass parent_class;
} PengePeopleTileClass;

GType penge_people_tile_get_type (void);
void penge_people_tile_activate (PengePeopleTile *tile,
                                 MojitoItem      *item);
G_END_DECLS

#endif /* _PENGE_PEOPLE_TILE */

