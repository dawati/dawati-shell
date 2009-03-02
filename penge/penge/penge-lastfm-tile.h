#ifndef _PENGE_LASTFM_TILE
#define _PENGE_LASTFM_TILE

#include <glib-object.h>
#include "penge-people-tile.h"

G_BEGIN_DECLS

#define PENGE_TYPE_LASTFM_TILE penge_lastfm_tile_get_type()

#define PENGE_LASTFM_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_LASTFM_TILE, PengeLastfmTile))

#define PENGE_LASTFM_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_LASTFM_TILE, PengeLastfmTileClass))

#define PENGE_IS_LASTFM_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_LASTFM_TILE))

#define PENGE_IS_LASTFM_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_LASTFM_TILE))

#define PENGE_LASTFM_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_LASTFM_TILE, PengeLastfmTileClass))

typedef struct {
  PengePeopleTile parent;
} PengeLastfmTile;

typedef struct {
  PengePeopleTileClass parent_class;
} PengeLastfmTileClass;

GType penge_lastfm_tile_get_type (void);

G_END_DECLS

#endif /* _PENGE_LASTFM_TILE */

