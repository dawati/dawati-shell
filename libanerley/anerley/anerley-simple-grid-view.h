#include <nbtk/nbtk.h>
#include <anerley/anerley-feed.h>

#ifndef _ANERLEY_SIMPLE_GRID_VIEW_H
#define _ANERLEY_SIMPLE_GRID_VIEW_H

#include <glib-object.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_SIMPLE_GRID_VIEW anerley_simple_grid_view_get_type()

#define ANERLEY_SIMPLE_GRID_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  ANERLEY_TYPE_SIMPLE_GRID_VIEW, AnerleySimpleGridView))

#define ANERLEY_SIMPLE_GRID_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  ANERLEY_TYPE_SIMPLE_GRID_VIEW, AnerleySimpleGridViewClass))

#define ANERLEY_IS_SIMPLE_GRID_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  ANERLEY_TYPE_SIMPLE_GRID_VIEW))

#define ANERLEY_IS_SIMPLE_GRID_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  ANERLEY_TYPE_SIMPLE_GRID_VIEW))

#define ANERLEY_SIMPLE_GRID_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  ANERLEY_TYPE_SIMPLE_GRID_VIEW, AnerleySimpleGridViewClass))

typedef struct {
  NbtkGrid parent;
} AnerleySimpleGridView;

typedef struct {
  NbtkGridClass parent_class;
} AnerleySimpleGridViewClass;

GType anerley_simple_grid_view_get_type (void);

NbtkWidget *anerley_simple_grid_view_new (AnerleyFeed *feed);

G_END_DECLS

#endif /* _ANERLEY_SIMPLE_GRID_VIEW_H */
