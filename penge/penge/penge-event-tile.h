#ifndef _PENGE_EVENT_TILE
#define _PENGE_EVENT_TILE

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define PENGE_TYPE_EVENT_TILE penge_event_tile_get_type()

#define PENGE_EVENT_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_EVENT_TILE, PengeEventTile))

#define PENGE_EVENT_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_EVENT_TILE, PengeEventTileClass))

#define PENGE_IS_EVENT_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_EVENT_TILE))

#define PENGE_IS_EVENT_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_EVENT_TILE))

#define PENGE_EVENT_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_EVENT_TILE, PengeEventTileClass))

typedef struct {
  NbtkTable parent;
} PengeEventTile;

typedef struct {
  NbtkTableClass parent_class;
} PengeEventTileClass;

GType penge_event_tile_get_type (void);
gchar *penge_event_tile_get_uid (PengeEventTile *tile);

G_END_DECLS

#endif /* _PENGE_EVENT_TILE */

