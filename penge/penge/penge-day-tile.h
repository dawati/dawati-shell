#ifndef _PENGE_DAY_TILE
#define _PENGE_DAY_TILE

#include <glib-object.h>

#include "penge-tile.h"

G_BEGIN_DECLS

#define PENGE_TYPE_DAY_TILE penge_day_tile_get_type()

#define PENGE_DAY_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_DAY_TILE, PengeDayTile))

#define PENGE_DAY_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_DAY_TILE, PengeDayTileClass))

#define PENGE_IS_DAY_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_DAY_TILE))

#define PENGE_IS_DAY_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_DAY_TILE))

#define PENGE_DAY_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_DAY_TILE, PengeDayTileClass))

typedef struct {
  PengeTile parent;
} PengeDayTile;

typedef struct {
  PengeTileClass parent_class;
} PengeDayTileClass;

GType penge_day_tile_get_type (void);

G_END_DECLS

#endif /* _PENGE_DAY_TILE */

