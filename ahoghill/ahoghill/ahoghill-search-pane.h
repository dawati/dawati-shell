#ifndef __AHOGHILL_SEARCH_PANE_H__
#define __AHOGHILL_SEARCH_PANE_H__

#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define AHOGHILL_TYPE_SEARCH_PANE                                       \
   (ahoghill_search_pane_get_type())
#define AHOGHILL_SEARCH_PANE(obj)                                       \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_SEARCH_PANE,              \
                                AhoghillSearchPane))
#define AHOGHILL_SEARCH_PANE_CLASS(klass)                               \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_SEARCH_PANE,                 \
                             AhoghillSearchPaneClass))
#define IS_AHOGHILL_SEARCH_PANE(obj)                                    \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_SEARCH_PANE))
#define IS_AHOGHILL_SEARCH_PANE_CLASS(klass)                            \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_SEARCH_PANE))
#define AHOGHILL_SEARCH_PANE_GET_CLASS(obj)                             \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_SEARCH_PANE,               \
                               AhoghillSearchPaneClass))

typedef struct _AhoghillSearchPanePrivate AhoghillSearchPanePrivate;
typedef struct _AhoghillSearchPane      AhoghillSearchPane;
typedef struct _AhoghillSearchPaneClass AhoghillSearchPaneClass;

struct _AhoghillSearchPane
{
    NbtkTable parent;

    AhoghillSearchPanePrivate *priv;
};

struct _AhoghillSearchPaneClass
{
    NbtkTableClass parent_class;
};

GType ahoghill_search_pane_get_type (void) G_GNUC_CONST;
NbtkWidget *ahoghill_search_pane_get_entry (AhoghillSearchPane *self);

G_END_DECLS

#endif /* __AHOGHILL_SEARCH_PANE_H__ */
