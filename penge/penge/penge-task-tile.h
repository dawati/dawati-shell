#ifndef _PENGE_TASK_TILE
#define _PENGE_TASK_TILE

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define PENGE_TYPE_TASK_TILE penge_task_tile_get_type()

#define PENGE_TASK_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_TASK_TILE, PengeTaskTile))

#define PENGE_TASK_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_TASK_TILE, PengeTaskTileClass))

#define PENGE_IS_TASK_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_TASK_TILE))

#define PENGE_IS_TASK_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_TASK_TILE))

#define PENGE_TASK_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_TASK_TILE, PengeTaskTileClass))

typedef struct {
  NbtkTable parent;
} PengeTaskTile;

typedef struct {
  NbtkTableClass parent_class;
} PengeTaskTileClass;

GType penge_task_tile_get_type (void);

gchar *penge_task_tile_get_uid (PengeTaskTile *tile);

G_END_DECLS

#endif /* _PENGE_TASK_TILE */

