#include <glib/gi18n.h>

#include "ahoghill-example-table.h"
#include "ahoghill-media-tile.h"

enum {
    PROP_0,
};

struct _AhoghillExampleTablePrivate {
    NbtkWidget *blurb;
    NbtkWidget *table;
    AhoghillMediaTile *tiles[5];
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_EXAMPLE_TABLE, AhoghillExampleTablePrivate))
G_DEFINE_TYPE (AhoghillExampleTable, ahoghill_example_table, NBTK_TYPE_TABLE);

static void
ahoghill_example_table_finalize (GObject *object)
{
    AhoghillExampleTable *self = (AhoghillExampleTable *) object;

    G_OBJECT_CLASS (ahoghill_example_table_parent_class)->finalize (object);
}

static void
ahoghill_example_table_dispose (GObject *object)
{
    AhoghillExampleTable *self = (AhoghillExampleTable *) object;

    G_OBJECT_CLASS (ahoghill_example_table_parent_class)->dispose (object);
}

static void
ahoghill_example_table_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    AhoghillExampleTable *self = (AhoghillExampleTable *) object;

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
    AhoghillExampleTable *self = (AhoghillExampleTable *) object;

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
}

static void
ahoghill_example_table_init (AhoghillExampleTable *self)
{
    AhoghillExampleTablePrivate *priv;
    ClutterText *text;

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    clutter_actor_set_size ((ClutterActor *) self, 700, 400);
    priv->blurb = nbtk_label_new (_("You need to play some music, look at some pictures or watch some video to see your history here. In the meantime, you could take a look at some sample media:"));
    text = nbtk_label_get_clutter_text ((NbtkLabel *) priv->blurb);
    clutter_text_set_line_wrap (text, TRUE);
    nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->blurb),
                                      "AhoghillExampleText");
    nbtk_table_add_actor_with_properties ((NbtkTable *) self,
                                          (ClutterActor *) priv->blurb,
                                          0, 0,
                                          "y-expand", FALSE,
                                          "x-align", 0.5,
                                          "y-align", 0.0,
                                          NULL);
}

