#ifndef _PENGE_TILE
#define _PENGE_TILE

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define PENGE_TYPE_TILE penge_tile_get_type()

#define PENGE_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_TILE, PengeTile))

#define PENGE_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_TILE, PengeTileClass))

#define PENGE_IS_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_TILE))

#define PENGE_IS_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_TILE))

#define PENGE_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_TILE, PengeTileClass))

typedef struct {
  ClutterActor parent;
} PengeTile;

typedef struct {
  ClutterActorClass parent_class;
} PengeTileClass;

GType penge_tile_get_type (void);

G_END_DECLS

#endif /* _PENGE_TILE */

