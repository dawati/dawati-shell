#ifndef _PENGE_MYSPACE_TILE
#define _PENGE_MYSPACE_TILE

#include <glib-object.h>
#include "penge-people-tile.h"

G_BEGIN_DECLS

#define PENGE_TYPE_MYSPACE_TILE penge_myspace_tile_get_type()

#define PENGE_MYSPACE_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_MYSPACE_TILE, PengeMyspaceTile))

#define PENGE_MYSPACE_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_MYSPACE_TILE, PengeMyspaceTileClass))

#define PENGE_IS_MYSPACE_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_MYSPACE_TILE))

#define PENGE_IS_MYSPACE_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_MYSPACE_TILE))

#define PENGE_MYSPACE_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_MYSPACE_TILE, PengeMyspaceTileClass))

typedef struct {
  PengePeopleTile parent;
} PengeMyspaceTile;

typedef struct {
  PengePeopleTileClass parent_class;
} PengeMyspaceTileClass;

GType penge_myspace_tile_get_type (void);

G_END_DECLS

#endif /* _PENGE_MYSPACE_TILE */

