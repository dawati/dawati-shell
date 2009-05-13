#include "ahoghill-media-tile.h"
#include "ahoghill-results-table.h"

enum {
    PROP_0,
    PROP_ROWS,
};

enum {
    ITEM_CLICKED,
    LAST_SIGNAL
};

#define TILES_PER_ROW 6
#define ROWS_PER_PAGE 2

#define RESULTS_ROW_SPACING 28
#define RESULTS_COL_SPACING 15

struct _AhoghillResultsTablePrivate {
    AhoghillMediaTile **tiles;
    AhoghillResultsModel *model;

    guint rows;
    guint update_id;
    guint page_number;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_RESULTS_TABLE, AhoghillResultsTablePrivate))
G_DEFINE_TYPE (AhoghillResultsTable, ahoghill_results_table, NBTK_TYPE_TABLE);

static guint32 signals[LAST_SIGNAL] = {0, };

static void
ahoghill_results_table_finalize (GObject *object)
{
    AhoghillResultsTable *table = (AhoghillResultsTable *) object;
    AhoghillResultsTablePrivate *priv = table->priv;

    if (priv->tiles) {
        g_free (priv->tiles);
        priv->tiles = NULL;
    }

    G_OBJECT_CLASS (ahoghill_results_table_parent_class)->finalize (object);
}

static void
ahoghill_results_table_dispose (GObject *object)
{
    AhoghillResultsTable *table = (AhoghillResultsTable *) object;
    AhoghillResultsTablePrivate *priv = table->priv;

    if (priv->model) {
        g_signal_handler_disconnect (priv->model, priv->update_id);
        g_object_unref (priv->model);
        priv->model = NULL;
    }

    G_OBJECT_CLASS (ahoghill_results_table_parent_class)->dispose (object);
}

static void
ahoghill_results_table_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
    AhoghillResultsTable *table = (AhoghillResultsTable *) object;
    AhoghillResultsTablePrivate *priv = table->priv;

    switch (prop_id) {

    case PROP_ROWS:
        priv->rows = g_value_get_int (value);
        break;

    default:
        break;
    }
}

static void
ahoghill_results_table_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    switch (prop_id) {

    default:
        break;
    }
}


static int
find_tile (AhoghillResultsTable *pane,
           NbtkWidget           *widget)
{
    AhoghillResultsTablePrivate *priv = pane->priv;
    int i, tiles_per_page;

    tiles_per_page = TILES_PER_ROW * priv->rows;
    for (i = 0; i < tiles_per_page; i++) {
        if (priv->tiles[i] == (AhoghillMediaTile *) widget) {
            return i;
        }
    }

    return -1;
}

static gboolean
tile_pressed_cb (ClutterActor         *actor,
                 ClutterButtonEvent   *event,
                 AhoghillResultsTable *table)
{
    int tileno;

    tileno = find_tile (table, (NbtkWidget *) actor);
    if (tileno == -1) {
        return TRUE;
    }

    return TRUE;
}

static gboolean
tile_released_cb (ClutterActor         *actor,
                  ClutterButtonEvent   *event,
                  AhoghillResultsTable *table)
{
    int tileno;

    tileno = find_tile (table, (NbtkWidget *) actor);
    if (tileno == -1) {
        return TRUE;
    }

    g_signal_emit (table, signals[ITEM_CLICKED], 0, tileno);

    return TRUE;
}

static void
tile_dnd_begin_cb (NbtkWidget           *widget,
                   ClutterActor         *dragged,
                   ClutterActor         *icon,
                   int                   x,
                   int                   y,
                   AhoghillResultsTable *table)
{
    int tileno;

    tileno = find_tile (table, widget);
    if (tileno == -1) {
        return;
    }
}
static void
tile_dnd_motion_cb (NbtkWidget           *widget,
                    ClutterActor         *dragged,
                    ClutterActor         *icon,
                    int                   x,
                    int                   y,
                    AhoghillResultsTable *table)
{
    int tileno;

    tileno = find_tile (table, widget);
    if (tileno == -1) {
        return;
    }
}

static void
tile_dnd_end_cb (NbtkWidget           *widget,
                 ClutterActor         *dragged,
                 ClutterActor         *icon,
                 int                   x,
                 int                   y,
                 AhoghillResultsTable *table)
{
    int tileno;

    tileno = find_tile (table, widget);
    if (tileno == -1) {
        return;
    }
}

static GObject *
ahoghill_results_table_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
    GObject *object;
    AhoghillResultsTable *table;
    AhoghillResultsTablePrivate *priv;
    int tiles_per_page, i;

    object = G_OBJECT_CLASS (ahoghill_results_table_parent_class)->constructor
        (type, n_params, params);
    table = (AhoghillResultsTable *) object;
    priv = table->priv;

    tiles_per_page = TILES_PER_ROW * priv->rows;

    priv->tiles = g_new (AhoghillMediaTile *, tiles_per_page);
    for (i = 0; i < tiles_per_page; i++) {
        priv->tiles[i] = g_object_new (AHOGHILL_TYPE_MEDIA_TILE, NULL);

        nbtk_widget_set_dnd_threshold (NBTK_WIDGET (priv->tiles[i]), 10);
        g_signal_connect (priv->tiles[i], "button-press-event",
                          G_CALLBACK (tile_pressed_cb), table);
        g_signal_connect (priv->tiles[i], "button-release-event",
                          G_CALLBACK (tile_released_cb), table);
        g_signal_connect (priv->tiles[i], "dnd-begin",
                          G_CALLBACK (tile_dnd_begin_cb), table);
        g_signal_connect (priv->tiles[i], "dnd-motion",
                          G_CALLBACK (tile_dnd_motion_cb), table);
        g_signal_connect (priv->tiles[i], "dnd-end",
                          G_CALLBACK (tile_dnd_end_cb), table);

        nbtk_table_add_actor_with_properties (NBTK_TABLE (table),
                                              (ClutterActor *) priv->tiles[i],
                                              (i / TILES_PER_ROW),
                                              i % TILES_PER_ROW,
                                              "x-align", 0.5,
                                              NULL);
        clutter_actor_hide ((ClutterActor *) priv->tiles[i]);
    }

    return object;
}

static void
ahoghill_results_table_class_init (AhoghillResultsTableClass *klass)
{
    GObjectClass *o_class = (GObjectClass *)klass;

    o_class->dispose = ahoghill_results_table_dispose;
    o_class->finalize = ahoghill_results_table_finalize;
    o_class->constructor = ahoghill_results_table_constructor;
    o_class->set_property = ahoghill_results_table_set_property;
    o_class->get_property = ahoghill_results_table_get_property;

    g_type_class_add_private (klass, sizeof (AhoghillResultsTablePrivate));

    g_object_class_install_property (o_class, PROP_ROWS,
                                     g_param_spec_int ("rows", "", "",
                                                       1, ROWS_PER_PAGE,
                                                       ROWS_PER_PAGE,
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS));

    signals[ITEM_CLICKED] = g_signal_new ("item-clicked",
                                          G_TYPE_FROM_CLASS (klass),
                                          G_SIGNAL_RUN_FIRST |
                                          G_SIGNAL_NO_RECURSE, 0, NULL, NULL,
                                          g_cclosure_marshal_VOID__INT,
                                          G_TYPE_NONE, 1,
                                          G_TYPE_INT);
}

static void
ahoghill_results_table_init (AhoghillResultsTable *self)
{
    AhoghillResultsTablePrivate *priv;

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    priv->rows = ROWS_PER_PAGE;

    clutter_actor_set_name (CLUTTER_ACTOR (self), "media-pane-results-table");
    nbtk_table_set_col_spacing (NBTK_TABLE (self), RESULTS_COL_SPACING);
    nbtk_table_set_row_spacing (NBTK_TABLE (self), RESULTS_ROW_SPACING);
}

static void
update_items (AhoghillResultsTable *self)
{
    AhoghillResultsTablePrivate *priv = self->priv;
    int i, count, start;
    int results_count, tiles_per_page;

    results_count = ahoghill_results_model_get_count (priv->model);

    tiles_per_page = TILES_PER_ROW * priv->rows;
    start = priv->page_number * tiles_per_page;
    count = MIN (results_count - start, tiles_per_page);

    for (i = 0; i < count; i++) {
        BklItem *item = ahoghill_results_model_get_item (priv->model,
                                                         i + start);

        g_object_set (priv->tiles[i],
                      "item", item,
                      NULL);
        clutter_actor_show ((ClutterActor *) priv->tiles[i]);
    }

    /* Clear the rest of the results */
    for (i = count; i < tiles_per_page; i++) {
        g_object_set (priv->tiles[i],
                      "item", NULL,
                      NULL);
        clutter_actor_hide ((ClutterActor *) priv->tiles[i]);
    }
}

static void
results_model_changed (AhoghillResultsModel *model,
                       AhoghillResultsTable *table)
{
    update_items (table);
}

static void
set_model (AhoghillResultsTable *table,
           AhoghillResultsModel *model)
{
    AhoghillResultsTablePrivate *priv;

    priv = table->priv;

    if (model) {
        priv->model = g_object_ref (model);
        priv->update_id = g_signal_connect (priv->model, "changed",
                                            G_CALLBACK (results_model_changed),
                                            table);
    }
}

AhoghillResultsTable *
ahoghill_results_table_new (AhoghillResultsModel *model,
                            int                   rows)
{
    AhoghillResultsTable *table;
    AhoghillResultsTablePrivate *priv;

    table = g_object_new (AHOGHILL_TYPE_RESULTS_TABLE,
                          "rows", rows,
                          NULL);
    priv = table->priv;

    set_model (table, model);

    return table;
}

void
ahoghill_results_table_set_model (AhoghillResultsTable *table,
                                  AhoghillResultsModel *model)
{
    g_return_if_fail (IS_AHOGHILL_RESULTS_TABLE (table));
    g_return_if_fail (IS_AHOGHILL_RESULTS_MODEL (model));

    set_model (table, model);
}

void
ahoghill_results_table_set_page (AhoghillResultsTable *self,
                                 guint                 page_number)
{
    AhoghillResultsTablePrivate *priv = self->priv;

    priv->page_number = page_number;
    update_items (self);
}
