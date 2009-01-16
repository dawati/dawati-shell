#ifndef _PENGE_GRID_VIEW
#define _PENGE_GRID_VIEW

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define PENGE_TYPE_GRID_VIEW penge_grid_view_get_type()

#define PENGE_GRID_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_GRID_VIEW, PengeGridView))

#define PENGE_GRID_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_GRID_VIEW, PengeGridViewClass))

#define PENGE_IS_GRID_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_GRID_VIEW))

#define PENGE_IS_GRID_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_GRID_VIEW))

#define PENGE_GRID_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_GRID_VIEW, PengeGridViewClass))

typedef struct {
  NbtkTable parent;
} PengeGridView;

typedef struct {
  NbtkTableClass parent_class;
} PengeGridViewClass;

GType penge_grid_view_get_type (void);

G_END_DECLS

#endif /* _PENGE_GRID_VIEW */

