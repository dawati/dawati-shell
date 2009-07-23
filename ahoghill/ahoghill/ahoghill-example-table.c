#include <glib/gi18n.h>

#include "ahoghill-example-table.h"
#include "ahoghill-results-table.h"

enum {
    PROP_0,
};

enum {
    ITEM_CLICKED,
    LAST_SIGNAL,
};

struct _AhoghillExampleTablePrivate {
    NbtkWidget *blurb;
    AhoghillResultsTable *table;
    AhoghillResultsModel *model;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_EXAMPLE_TABLE, AhoghillExampleTablePrivate))
G_DEFINE_TYPE (AhoghillExampleTable, ahoghill_example_table, NBTK_TYPE_TABLE);
static guint32 signals[LAST_SIGNAL] = {0, };

static void
ahoghill_example_table_finalize (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_example_table_parent_class)->finalize (object);
}

static void
ahoghill_example_table_dispose (GObject *object)
{
    AhoghillExampleTable *self = (AhoghillExampleTable *) object;
    AhoghillExampleTablePrivate *priv = self->priv;

    if (priv->model) {
        g_object_unref (priv->model);
        priv->model = NULL;
    }

    G_OBJECT_CLASS (ahoghill_example_table_parent_class)->dispose (object);
}

static void
ahoghill_example_table_set_property (GObject      *object,
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
ahoghill_example_table_get_property (GObject    *object,
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
ahoghill_example_table_class_init (AhoghillExampleTableClass *klass)
{
    GObjectClass *o_class = (GObjectClass *)klass;

    o_class->dispose = ahoghill_example_table_dispose;
    o_class->finalize = ahoghill_example_table_finalize;
    o_class->set_property = ahoghill_example_table_set_property;
    o_class->get_property = ahoghill_example_table_get_property;

    g_type_class_add_private (klass, sizeof (AhoghillExampleTablePrivate));

    signals[ITEM_CLICKED] = g_signal_new ("item-clicked",
                                          G_TYPE_FROM_CLASS (klass),
                                          G_SIGNAL_RUN_FIRST |
                                          G_SIGNAL_NO_RECURSE, 0, NULL, NULL,
                                          g_cclosure_marshal_VOID__INT,
                                          G_TYPE_NONE, 1,
                                          G_TYPE_INT);
}

static void
item_clicked_cb (AhoghillResultsTable *table,
                 int                   item_no,
                 AhoghillExampleTable *example)
{
    g_signal_emit (example, signals[ITEM_CLICKED], 0, item_no);
}

static void
ahoghill_example_table_init (AhoghillExampleTable *self)
{
    AhoghillExampleTablePrivate *priv;
    ClutterActor *text;
    NbtkWidget *bin;

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    clutter_actor_set_size ((ClutterActor *) self, 700, 400);
    bin = nbtk_bin_new ();
    nbtk_bin_set_alignment (NBTK_BIN (bin),
                            NBTK_ALIGN_LEFT,
                            NBTK_ALIGN_CENTER);
    nbtk_widget_set_style_class_name (NBTK_WIDGET (bin),
                                      "AhoghillExampleTextBin");
    priv->blurb = nbtk_label_new (_("You need to play some music, look at some pictures or watch some video to see your history here. In the meantime, you could take a look at some sample media:"));
    nbtk_bin_set_child (NBTK_BIN (bin), (ClutterActor *) priv->blurb);
    text = nbtk_label_get_clutter_text ((NbtkLabel *) priv->blurb);
    clutter_text_set_line_wrap ((ClutterText *) text, TRUE);
    nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->blurb),
                                      "AhoghillExampleText");
    nbtk_table_add_actor_with_properties ((NbtkTable *) self,
                                          (ClutterActor *)bin,
                                          0, 0,
                                          "y-expand", FALSE,
                                          "x-expand", TRUE,
                                          "x-fill", TRUE,
                                          "y-fill", TRUE,
                                          NULL);

    priv->table = ahoghill_results_table_new (NULL, 1);
    nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->table),
                                      "AhoghillExampleResults");
    g_signal_connect (priv->table, "item-clicked",
                      G_CALLBACK (item_clicked_cb), self);

    nbtk_table_add_actor_with_properties ((NbtkTable *) self,
                                          (ClutterActor *) priv->table,
                                          1, 0,
                                          "x-expand", FALSE,
                                          "y-expand", FALSE,
                                          "x-align", 0.5,
                                          "y-align", 0.5,
                                          NULL);
}

AhoghillExampleTable *
ahoghill_example_table_new (AhoghillResultsModel *model)
{
    AhoghillExampleTable *table;
    AhoghillExampleTablePrivate *priv;

    table = g_object_new (AHOGHILL_TYPE_EXAMPLE_TABLE, NULL);
    priv = table->priv;

    priv->model = g_object_ref (model);
    ahoghill_results_table_set_model (priv->table, priv->model);

    return table;
}
