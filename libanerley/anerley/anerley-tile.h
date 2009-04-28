#ifndef _ANERLEY_TILE
#define _ANERLEY_TILE

#include <glib-object.h>
#include <anerley/anerley-item.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_TILE anerley_tile_get_type()

#define ANERLEY_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANERLEY_TYPE_TILE, AnerleyTile))

#define ANERLEY_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), ANERLEY_TYPE_TILE, AnerleyTileClass))

#define ANERLEY_IS_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANERLEY_TYPE_TILE))

#define ANERLEY_IS_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANERLEY_TYPE_TILE))

#define ANERLEY_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANERLEY_TYPE_TILE, AnerleyTileClass))

typedef struct {
  NbtkTable parent;
} AnerleyTile;

typedef struct {
  NbtkTableClass parent_class;
} AnerleyTileClass;

GType anerley_tile_get_type (void);
NbtkWidget *anerley_tile_new (AnerleyItem *item);

G_END_DECLS

#endif /* _ANERLEY_TILE */

