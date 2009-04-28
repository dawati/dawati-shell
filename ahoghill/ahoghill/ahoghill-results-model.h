#ifndef __AHOGHILL_RESULTS_MODEL_H__
#define __AHOGHILL_RESULTS_MODEL_H__

#include <glib-object.h>

#include <bickley/bkl-item.h>

G_BEGIN_DECLS

#define AHOGHILL_TYPE_RESULTS_MODEL                                     \
   (ahoghill_results_model_get_type())
#define AHOGHILL_RESULTS_MODEL(obj)                                     \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_RESULTS_MODEL,            \
                                AhoghillResultsModel))
#define AHOGHILL_RESULTS_MODEL_CLASS(klass)                             \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_RESULTS_MODEL,               \
                             AhoghillResultsModelClass))
#define IS_AHOGHILL_RESULTS_MODEL(obj)                                  \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_RESULTS_MODEL))
#define IS_AHOGHILL_RESULTS_MODEL_CLASS(klass)                          \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_RESULTS_MODEL))
#define AHOGHILL_RESULTS_MODEL_GET_CLASS(obj)                           \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_RESULTS_MODEL,             \
                               AhoghillResultsModelClass))

typedef struct _AhoghillResultsModelPrivate AhoghillResultsModelPrivate;
typedef struct _AhoghillResultsModel      AhoghillResultsModel;
typedef struct _AhoghillResultsModelClass AhoghillResultsModelClass;

struct _AhoghillResultsModel
{
    GObject parent;

    AhoghillResultsModelPrivate *priv;
};

struct _AhoghillResultsModelClass
{
    GObjectClass parent_class;
};

GType ahoghill_results_model_get_type (void) G_GNUC_CONST;

void ahoghill_results_model_add_item (AhoghillResultsModel *model,
                                      BklItem              *item);
void ahoghill_results_model_remove_item (AhoghillResultsModel *model,
                                         BklItem              *item);

G_END_DECLS

#endif /* __AHOGHILL_RESULTS_MODEL_H__ */
