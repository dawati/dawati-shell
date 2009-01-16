#ifndef _PENGE_DATE_TILE
#define _PENGE_DATE_TILE

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define PENGE_TYPE_DATE_TILE penge_date_tile_get_type()

#define PENGE_DATE_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_DATE_TILE, PengeDateTile))

#define PENGE_DATE_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_DATE_TILE, PengeDateTileClass))

#define PENGE_IS_DATE_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_DATE_TILE))

#define PENGE_IS_DATE_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_DATE_TILE))

#define PENGE_DATE_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_DATE_TILE, PengeDateTileClass))

typedef struct {
  NbtkTable parent;
} PengeDateTile;

typedef struct {
  NbtkTableClass parent_class;
} PengeDateTileClass;

GType penge_date_tile_get_type (void);

G_END_DECLS

#endif /* _PENGE_DATE_TILE */

