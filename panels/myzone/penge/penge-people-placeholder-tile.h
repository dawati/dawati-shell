 /*
 * Copyright (C) 2009 Intel Corporation.
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

#ifndef _PENGE_PEOPLE_PLACEHOLDER_TILE
#define _PENGE_PEOPLE_PLACEHOLDER_TILE

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define PENGE_TYPE_PEOPLE_PLACEHOLDER_TILE penge_people_placeholder_tile_get_type()

#define PENGE_PEOPLE_PLACEHOLDER_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_PEOPLE_PLACEHOLDER_TILE, PengePeoplePlaceholderTile))

#define PENGE_PEOPLE_PLACEHOLDER_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_PEOPLE_PLACEHOLDER_TILE, PengePeoplePlaceholderTileClass))

#define PENGE_IS_PEOPLE_PLACEHOLDER_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_PEOPLE_PLACEHOLDER_TILE))

#define PENGE_IS_PEOPLE_PLACEHOLDER_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_PEOPLE_PLACEHOLDER_TILE))

#define PENGE_PEOPLE_PLACEHOLDER_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_PEOPLE_PLACEHOLDER_TILE, PengePeoplePlaceholderTileClass))

typedef struct {
  NbtkButton parent;
} PengePeoplePlaceholderTile;

typedef struct {
  NbtkButtonClass parent_class;
} PengePeoplePlaceholderTileClass;

GType penge_people_placeholder_tile_get_type (void);

ClutterActor *penge_people_placeholder_tile_new (void);

G_END_DECLS

#endif /* _PENGE_PEOPLE_PLACEHOLDER_TILE */

