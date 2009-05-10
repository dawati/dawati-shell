#ifndef _PENGE_APP_TILE
#define _PENGE_APP_TILE

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define PENGE_TYPE_APP_TILE penge_app_tile_get_type()

#define PENGE_APP_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_APP_TILE, PengeAppTile))

#define PENGE_APP_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_APP_TILE, PengeAppTileClass))

#define PENGE_IS_APP_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_APP_TILE))

#define PENGE_IS_APP_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_APP_TILE))

#define PENGE_APP_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_APP_TILE, PengeAppTileClass))

typedef struct {
  NbtkButton parent;
} PengeAppTile;

typedef struct {
  NbtkButtonClass parent_class;
} PengeAppTileClass;

GType penge_app_tile_get_type (void);

G_END_DECLS

#endif /* _PENGE_APP_TILE */

