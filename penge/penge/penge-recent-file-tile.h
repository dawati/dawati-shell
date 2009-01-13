#ifndef _PENGE_RECENT_FILE_TILE
#define _PENGE_RECENT_FILE_TILE

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define PENGE_TYPE_RECENT_FILE_TILE penge_recent_file_tile_get_type()

#define PENGE_RECENT_FILE_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_RECENT_FILE_TILE, PengeRecentFileTile))

#define PENGE_RECENT_FILE_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_RECENT_FILE_TILE, PengeRecentFileTileClass))

#define PENGE_IS_RECENT_FILE_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_RECENT_FILE_TILE))

#define PENGE_IS_RECENT_FILE_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_RECENT_FILE_TILE))

#define PENGE_RECENT_FILE_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_RECENT_FILE_TILE, PengeRecentFileTileClass))

typedef struct {
  NbtkWidget parent;
} PengeRecentFileTile;

typedef struct {
  NbtkWidgetClass parent_class;
} PengeRecentFileTileClass;

GType penge_recent_file_tile_get_type (void);

const gchar *penge_recent_file_tile_get_uri (PengeRecentFileTile *tile);
G_END_DECLS

#endif /* _PENGE_RECENT_FILE_TILE */

