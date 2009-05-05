#ifndef __AHOGHILL_RESULTS_TABLE_H__
#define __AHOGHILL_RESULTS_TABLE_H__

#include <nbtk/nbtk.h>

#include "ahoghill-results-model.h"

G_BEGIN_DECLS

#define AHOGHILL_TYPE_RESULTS_TABLE                                     \
   (ahoghill_results_table_get_type())
#define AHOGHILL_RESULTS_TABLE(obj)                                     \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_RESULTS_TABLE,            \
                                AhoghillResultsTable))
#define AHOGHILL_RESULTS_TABLE_CLASS(klass)                             \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_RESULTS_TABLE,               \
                             AhoghillResultsTableClass))
#define IS_AHOGHILL_RESULTS_TABLE(obj)                                  \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_RESULTS_TABLE))
#define IS_AHOGHILL_RESULTS_TABLE_CLASS(klass)                          \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_RESULTS_TABLE))
#define AHOGHILL_RESULTS_TABLE_GET_CLASS(obj)                           \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_RESULTS_TABLE,             \
                               AhoghillResultsTableClass))

typedef struct _AhoghillResultsTablePrivate AhoghillResultsTablePrivate;
typedef struct _AhoghillResultsTable      AhoghillResultsTable;
typedef struct _AhoghillResultsTableClass AhoghillResultsTableClass;

struct _AhoghillResultsTable
{
    NbtkTable parent;

    AhoghillResultsTablePrivate *priv;
};

struct _AhoghillResultsTableClass
{
    NbtkTableClass parent_class;
};

GType ahoghill_results_table_get_type (void) G_GNUC_CONST;

AhoghillResultsTable *ahoghill_results_table_new (AhoghillResultsModel *model);
void ahoghill_results_table_set_model (AhoghillResultsTable *table,
                                       AhoghillResultsModel *model);
void ahoghill_results_table_set_page (AhoghillResultsTable *table,
                                      guint                 page_number);

G_END_DECLS

#endif /* __AHOGHILL_RESULTS_TABLE_H__ */
