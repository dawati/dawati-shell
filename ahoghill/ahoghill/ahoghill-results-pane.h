#ifndef __AHOGHILL_RESULTS_PANE_H__
#define __AHOGHILL_RESULTS_PANE_H__

#include <nbtk/nbtk-table.h>


G_BEGIN_DECLS

#define AHOGHILL_TYPE_RESULTS_PANE                                      \
   (ahoghill_results_pane_get_type())
#define AHOGHILL_RESULTS_PANE(obj)                                      \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_RESULTS_PANE,             \
                                AhoghillResultsPane))
#define AHOGHILL_RESULTS_PANE_CLASS(klass)                              \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_RESULTS_PANE,                \
                             AhoghillResultsPaneClass))
#define IS_AHOGHILL_RESULTS_PANE(obj)                                   \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_RESULTS_PANE))
#define IS_AHOGHILL_RESULTS_PANE_CLASS(klass)                           \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_RESULTS_PANE))
#define AHOGHILL_RESULTS_PANE_GET_CLASS(obj)                            \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_RESULTS_PANE,              \
                               AhoghillResultsPaneClass))

typedef struct _AhoghillResultsPanePrivate AhoghillResultsPanePrivate;
typedef struct _AhoghillResultsPane      AhoghillResultsPane;
typedef struct _AhoghillResultsPaneClass AhoghillResultsPaneClass;

struct _AhoghillResultsPane
{
    NbtkTable parent;

    AhoghillResultsPanePrivate *priv;
};

struct _AhoghillResultsPaneClass
{
    NbtkTableClass parent_class;
};

GType ahoghill_results_pane_get_type (void) G_GNUC_CONST;
void ahoghill_results_pane_set_results (AhoghillResultsPane *self,
                                        GPtrArray           *results);

G_END_DECLS

#endif /* __AHOGHILL_RESULTS_PANE_H__ */
