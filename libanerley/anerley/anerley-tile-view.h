#ifndef _ANERLEY_TILE_VIEW
#define _ANERLEY_TILE_VIEW

#include <nbtk/nbtk.h>
#include <anerley/anerley-feed-model.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_TILE_VIEW anerley_tile_view_get_type()

#define ANERLEY_TILE_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANERLEY_TYPE_TILE_VIEW, AnerleyTileView))

#define ANERLEY_TILE_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), ANERLEY_TYPE_TILE_VIEW, AnerleyTileViewClass))

#define ANERLEY_IS_TILE_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANERLEY_TYPE_TILE_VIEW))

#define ANERLEY_IS_TILE_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANERLEY_TYPE_TILE_VIEW))

#define ANERLEY_TILE_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANERLEY_TYPE_TILE_VIEW, AnerleyTileViewClass))

typedef struct {
  NbtkIconView parent;
} AnerleyTileView;

typedef struct {
  NbtkIconViewClass parent_class;
} AnerleyTileViewClass;

GType anerley_tile_view_get_type (void);

NbtkWidget *anerley_tile_view_new (AnerleyFeedModel *model);

G_END_DECLS

#endif /* _ANERLEY_TILE_VIEW */

