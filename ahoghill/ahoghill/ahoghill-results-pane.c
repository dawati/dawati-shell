#include <glib/gi18n.h>

#include <nbtk/nbtk-label.h>

#include "ahoghill-media-tile.h"
#include "ahoghill-results-pane.h"

enum {
    PROP_0,
};

struct _AhoghillResultsPanePrivate {
    NbtkWidget *title;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_RESULTS_PANE, AhoghillResultsPanePrivate))
G_DEFINE_TYPE (AhoghillResultsPane, ahoghill_results_pane, NBTK_TYPE_TABLE);

#define ROW_SPACING 28
#define COL_SPACING 20

static void
ahoghill_results_pane_finalize (GObject *object)
{
    g_signal_handlers_destroy (object);
    G_OBJECT_CLASS (ahoghill_results_pane_parent_class)->finalize (object);
}

static void
ahoghill_results_pane_dispose (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_results_pane_parent_class)->dispose (object);
}

static void
ahoghill_results_pane_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    switch (prop_id) {

    default:
        break;
    }
}

static void
ahoghill_results_pane_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    switch (prop_id) {

    default:
        break;
    }
}

static void
ahoghill_results_pane_class_init (AhoghillResultsPaneClass *klass)
{
    GObjectClass *o_class = (GObjectClass *)klass;

    o_class->dispose = ahoghill_results_pane_dispose;
    o_class->finalize = ahoghill_results_pane_finalize;
    o_class->set_property = ahoghill_results_pane_set_property;
    o_class->get_property = ahoghill_results_pane_get_property;

    g_type_class_add_private (klass, sizeof (AhoghillResultsPanePrivate));
}

static void
ahoghill_results_pane_init (AhoghillResultsPane *self)
{
    AhoghillResultsPanePrivate *priv = GET_PRIVATE (self);

    self->priv = priv;

    clutter_actor_set_name (CLUTTER_ACTOR (self), "media-pane-results");
    nbtk_table_set_col_spacing (NBTK_TABLE (self), COL_SPACING);
    nbtk_table_set_row_spacing (NBTK_TABLE (self), ROW_SPACING);

    priv->title = nbtk_label_new (_("Recent"));
    clutter_actor_set_name (CLUTTER_ACTOR (priv->title), "media-pane-results-label");
    nbtk_table_add_widget_full (NBTK_TABLE (self), priv->title,
                                0, 0, 1, 6,
                                0, 0.0, 0.5);
}

void
ahoghill_results_pane_set_results (AhoghillResultsPane *self,
                                   GPtrArray           *results)
{
    int i;

    /* 6 columns */
    for (i = 0; i < results->len; i++) {
        NbtkWidget *widget = g_object_new (AHOGHILL_TYPE_MEDIA_TILE,
                                          "item", results->pdata[i],
                                          NULL);

        nbtk_table_add_widget_full (NBTK_TABLE (self), widget,
                                    (i / 6) + 1, i % 6, 1, 1, 0, 0.5, 0.5);
    }
}
