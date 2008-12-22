#ifndef _PENGE_PEOPLE_TILE
#define _PENGE_PEOPLE_TILE

#include <nbtk/nbtk.h>
#include <glib-object.h>

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

G_END_DECLS

#endif /* _PENGE_PEOPLE_TILE */

