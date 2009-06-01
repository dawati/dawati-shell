#include <glib.h>

#include "ahoghill-results-model.h"

enum {
    CHANGED,
    LAST_SIGNAL,
};

struct _ResultItem {
    BklSourceClient *owner;
    BklItem *item;
};

struct _AhoghillResultsModelPrivate {
    GPtrArray *items; /* Of type struct _ResultItem */

    gboolean dirty;
    gboolean frozen;
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
    int i;

    for (i = 0; i < priv->items->len; i++) {
        struct _ResultItem *ri = priv->items->pdata[i];

        if (ri->item == (BklItem *) dead_object) {
            g_slice_free (struct _ResultItem, ri);

            g_ptr_array_remove_index (priv->items, i);
            break;
        }
    }

    if (priv->frozen) {
        priv->dirty = TRUE;
    } else {
        g_signal_emit (model, signals[CHANGED], 0);
    }
}

void
ahoghill_results_model_add_item (AhoghillResultsModel *model,
                                 BklSourceClient      *owner,
                                 BklItem              *item)
{
    AhoghillResultsModelPrivate *priv = model->priv;
    struct _ResultItem *ri;

    ri = g_slice_new (struct _ResultItem);
    ri->owner = owner;
    ri->item = item;

    g_ptr_array_add (priv->items, ri);
    g_object_weak_ref ((GObject *) item, remove_item, model);

    if (priv->frozen) {
        priv->dirty = TRUE;
    } else {
        g_signal_emit (model, signals[CHANGED], 0);
    }
}

void
ahoghill_results_model_remove_item (AhoghillResultsModel *model,
                                    BklItem              *item)
{
    g_object_weak_unref ((GObject *) item, remove_item, model);
    remove_item (model, (GObject *) item);
}

BklItem *
ahoghill_results_model_get_item (AhoghillResultsModel *model,
                                 int                   item_no)
{
    AhoghillResultsModelPrivate *priv;
    struct _ResultItem *ri;

    priv = model->priv;

    /* Check we have that many items */
    if (item_no >= priv->items->len) {
        return NULL;
    }

    ri = priv->items->pdata[item_no];
    return ri->item;
}

int
ahoghill_results_model_get_count (AhoghillResultsModel *model)
{
    AhoghillResultsModelPrivate *priv;

    priv = model->priv;

    return priv->items->len;
}

void
ahoghill_results_model_clear (AhoghillResultsModel *model)
{
    AhoghillResultsModelPrivate *priv;
    int i;

    priv = model->priv;
    for (i = 0; i < priv->items->len; i++) {
        struct _ResultItem *ri = priv->items->pdata[i];
        g_object_weak_unref ((GObject *) ri->item, remove_item, model);
        g_slice_free (struct _ResultItem, ri);
    }

    g_ptr_array_free (priv->items, TRUE);
    priv->items = g_ptr_array_new ();

    if (priv->frozen) {
        priv->dirty = TRUE;
    } else {
        g_signal_emit (model, signals[CHANGED], 0);
    }
}

void
ahoghill_results_model_freeze (AhoghillResultsModel *model)
{
    AhoghillResultsModelPrivate *priv;

    priv = model->priv;
    priv->frozen = TRUE;
    priv->dirty = FALSE;
}

void
ahoghill_results_model_thaw (AhoghillResultsModel *model)
{
    AhoghillResultsModelPrivate *priv;

    priv = model->priv;

    if (priv->frozen && priv->dirty) {
        priv->frozen = FALSE;
        priv->dirty = FALSE;

        g_signal_emit (model, signals[CHANGED], 0);
    }
}

/* Remove all the items that are owned by @owner */
void
ahoghill_results_model_remove_source_items (AhoghillResultsModel *model,
                                            BklSourceClient      *owner)
{
    AhoghillResultsModelPrivate *priv = model->priv;
    gboolean changed = FALSE;
    int i;

    for (i = priv->items->len - 1; i >= 0; i--) {
        struct _ResultItem *ri = priv->items->pdata[i];

        if (ri->owner == owner) {
            g_slice_free (struct _ResultItem, ri);

            g_object_weak_unref ((GObject *) ri->item, remove_item, model);
            g_ptr_array_remove_index (priv->items, i);
            changed = TRUE;
        }
    }

    if (changed) {
        g_signal_emit (model, signals[CHANGED], 0);
    }
}
