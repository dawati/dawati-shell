#ifndef __AHOGHILL_GRID_VIEW_H__
#define __AHOGHILL_GRID_VIEW_H__

#include <nbtk/nbtk.h>


G_BEGIN_DECLS

#define AHOGHILL_TYPE_GRID_VIEW                                         \
   (ahoghill_grid_view_get_type())
#define AHOGHILL_GRID_VIEW(obj)                                         \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_GRID_VIEW,                \
                                AhoghillGridView))
#define AHOGHILL_GRID_VIEW_CLASS(klass)                                 \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_GRID_VIEW,                   \
                             AhoghillGridViewClass))
#define IS_AHOGHILL_GRID_VIEW(obj)                                      \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_GRID_VIEW))
#define IS_AHOGHILL_GRID_VIEW_CLASS(klass)                              \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_GRID_VIEW))
#define AHOGHILL_GRID_VIEW_GET_CLASS(obj)                               \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_GRID_VIEW,                 \
                               AhoghillGridViewClass))

typedef struct _AhoghillGridViewPrivate AhoghillGridViewPrivate;
typedef struct _AhoghillGridView      AhoghillGridView;
typedef struct _AhoghillGridViewClass AhoghillGridViewClass;

struct _AhoghillGridView
{
    NbtkTable parent;

    AhoghillGridViewPrivate *priv;
};

struct _AhoghillGridViewClass
{
    NbtkTableClass parent_class;
};

GType ahoghill_grid_view_get_type (void) G_GNUC_CONST;
void ahoghill_grid_view_clear (AhoghillGridView *view);
void ahoghill_grid_view_focus (AhoghillGridView *view);
void ahoghill_grid_view_unfocus (AhoghillGridView *view);

G_END_DECLS

#endif /* __AHOGHILL_GRID_VIEW_H__ */
