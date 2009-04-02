#include <config.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "ahoghill-search-pane.h"
#include <src/mnb-entry.h>

enum {
    PROP_0,
};

struct _AhoghillSearchPanePrivate {
    NbtkWidget *entry;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_SEARCH_PANE, AhoghillSearchPanePrivate))
G_DEFINE_TYPE (AhoghillSearchPane, ahoghill_search_pane, NBTK_TYPE_TABLE);

#define WIDGET_SPACING 5
#define PADDING 8

static void
ahoghill_search_pane_finalize (GObject *object)
{
    g_signal_handlers_destroy (object);
    G_OBJECT_CLASS (ahoghill_search_pane_parent_class)->finalize (object);
}

static void
ahoghill_search_pane_dispose (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_search_pane_parent_class)->dispose (object);
}

static void
ahoghill_search_pane_set_property (GObject      *object,
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
ahoghill_search_pane_get_property (GObject    *object,
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
ahoghill_search_pane_class_init (AhoghillSearchPaneClass *klass)
{
    GObjectClass *o_class = (GObjectClass *)klass;

    o_class->dispose = ahoghill_search_pane_dispose;
    o_class->finalize = ahoghill_search_pane_finalize;
    o_class->set_property = ahoghill_search_pane_set_property;
    o_class->get_property = ahoghill_search_pane_get_property;

    g_type_class_add_private (klass, sizeof (AhoghillSearchPanePrivate));
}

static void
ahoghill_search_pane_init (AhoghillSearchPane *self)
{
    AhoghillSearchPanePrivate *priv = GET_PRIVATE (self);
    NbtkTable *table = (NbtkTable *) self;
    NbtkWidget *label;

    self->priv = priv;
    clutter_actor_set_name (CLUTTER_ACTOR (self), "media-pane-search");
    nbtk_table_set_col_spacing (NBTK_TABLE (self), WIDGET_SPACING);

    label = nbtk_label_new (_("Media"));
    clutter_actor_set_name (CLUTTER_ACTOR (label), "media-pane-search-label");
    nbtk_table_add_widget_full (table, label, 0, 0, 1, 1,
                                NBTK_Y_EXPAND, 0.0, 0.5);

    priv->entry = mnb_entry_new (_("Search"));
    clutter_actor_set_name (CLUTTER_ACTOR (priv->entry),
                            "media-pane-search-entry");
    clutter_actor_set_width (CLUTTER_ACTOR (priv->entry),
                             CLUTTER_UNITS_FROM_DEVICE (600));
    nbtk_table_add_widget_full (table, priv->entry, 0, 1, 1, 1,
                                NBTK_Y_EXPAND | NBTK_Y_FILL, 0.0, 0.5);
}

NbtkWidget *
ahoghill_search_pane_get_entry (AhoghillSearchPane *self)
{
    g_return_val_if_fail (IS_AHOGHILL_SEARCH_PANE (self), NULL);

    return self->priv->entry;
}
