#include <glib.h>

#include "ahoghill-results-model.h"

enum {
    CHANGED,
    LAST_SIGNAL,
};

struct _AhoghillResultsModelPrivate {
    GPtrArray *items;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_RESULTS_MODEL, AhoghillResultsModelPrivate))
G_DEFINE_TYPE (AhoghillResultsModel, ahoghill_results_model, G_TYPE_OBJECT);
static guint32 signals[LAST_SIGNAL] = {0, };

static void remove_item (gpointer data,
                         GObject *dead_object);

static void
ahoghill_results_model_finalize (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_results_model_parent_class)->finalize (object);
}

static void
ahoghill_results_model_dispose (GObject *object)
{
    AhoghillResultsModel *self = (AhoghillResultsModel *) object;
    AhoghillResultsModelPrivate *priv = self->priv;

    if (priv->items) {
        int i;

        for (i = 0; i < priv->items->len; i++) {
            g_object_weak_unref ((GObject *) priv->items->pdata[i],
                                 remove_item, self);
        }
        g_ptr_array_free (priv->items, TRUE);
        priv->items = NULL;
    }

    G_OBJECT_CLASS (ahoghill_results_model_parent_class)->dispose (object);
}

static void
ahoghill_results_model_class_init (AhoghillResultsModelClass *klass)
{
    GObjectClass *o_class = (GObjectClass *)klass;

    o_class->dispose = ahoghill_results_model_dispose;
    o_class->finalize = ahoghill_results_model_finalize;

    g_type_class_add_private (klass, sizeof (AhoghillResultsModelPrivate));

    signals[CHANGED] = g_signal_new ("changed",
                                     G_TYPE_FROM_CLASS (klass),
                                     G_SIGNAL_RUN_FIRST |
                                     G_SIGNAL_NO_RECURSE, 0, NULL, NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE, 0);
}

static void
ahoghill_results_model_init (AhoghillResultsModel *self)
{
    AhoghillResultsModelPrivate *priv;

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    priv->items = g_ptr_array_new ();
}

static void
remove_item (gpointer data,
             GObject *dead_object)
{
    AhoghillResultsModel *model = (AhoghillResultsModel *) data;
    AhoghillResultsModelPrivate *priv = model->priv;

    g_ptr_array_remove (priv->items, dead_object);
    g_signal_emit (model, signals[CHANGED], 0);
}

void
ahoghill_results_model_add_item (AhoghillResultsModel *model,
                                 BklItem              *item)
{
    AhoghillResultsModelPrivate *priv = model->priv;

    g_ptr_array_add (priv->items, item);
    g_object_weak_ref ((GObject *) item, remove_item, model);

    g_signal_emit (model, signals[CHANGED], 0);
}

void
ahoghill_results_model_remove_item (AhoghillResultsModel *model,
                                    BklItem              *item)
{
    g_object_weak_unref ((GObject *) item, remove_item, model);
    remove_item (model, (GObject *) item);
}
