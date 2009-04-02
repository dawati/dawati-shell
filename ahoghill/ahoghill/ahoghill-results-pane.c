#include <glib/gi18n.h>

#include <nbtk/nbtk-label.h>

#include <bickley/bkl-item.h>

#include "ahoghill-media-tile.h"
#include "ahoghill-results-pane.h"

enum {
    PROP_0,
};

enum {
    ITEM_CLICKED,
    LAST_SIGNAL
};

#define TILES_PER_ROW 6
#define ROWS_PER_PAGE 2
#define TILES_PER_PAGE TILES_PER_ROW * ROWS_PER_PAGE

struct _AhoghillResultsPanePrivate {
    NbtkWidget *title;
    AhoghillMediaTile *tiles[TILES_PER_PAGE];

    guint results_page;
    GPtrArray *results;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_RESULTS_PANE, AhoghillResultsPanePrivate))
G_DEFINE_TYPE (AhoghillResultsPane, ahoghill_results_pane, NBTK_TYPE_TABLE);

#define ROW_SPACING 28
#define COL_SPACING 20

static guint32 signals[LAST_SIGNAL] = { 0, };

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

    signals[ITEM_CLICKED] = g_signal_new ("item-clicked",
                                          G_TYPE_FROM_CLASS (klass),
                                          G_SIGNAL_RUN_FIRST |
                                          G_SIGNAL_NO_RECURSE, 0, NULL, NULL,
                                          g_cclosure_marshal_VOID__OBJECT,
                                          G_TYPE_NONE, 1,
                                          BKL_TYPE_ITEM);
}

static int
find_tile (AhoghillResultsPane *pane,
           NbtkWidget          *widget)
{
    AhoghillResultsPanePrivate *priv = pane->priv;
    int i;

    for (i = 0; i < TILES_PER_PAGE; i++) {
        if (priv->tiles[i] == (AhoghillMediaTile *) widget) {
            return i;
        }
    }

    return -1;
}

static gboolean
tile_pressed_cb (ClutterActor        *actor,
                 ClutterButtonEvent  *event,
                 AhoghillResultsPane *pane)
{
    int tileno;

    tileno = find_tile (pane, (NbtkWidget *) actor);
    if (tileno == -1) {
        return TRUE;
    }

    return TRUE;
}

static gboolean
tile_released_cb (ClutterActor        *actor,
                  ClutterButtonEvent  *event,
                  AhoghillResultsPane *pane)
{
    AhoghillResultsPanePrivate *priv = pane->priv;
    int tileno;
    BklItem *item;

    tileno = find_tile (pane, (NbtkWidget *) actor);
    if (tileno == -1) {
        return TRUE;
    }

    /* FIXME: When we have paging, remember to add the offset here */
    item = priv->results->pdata[tileno];

    g_signal_emit (pane, signals[ITEM_CLICKED], 0, item);

    return TRUE;
}

static void
tile_dnd_begin_cb (NbtkWidget          *widget,
                   ClutterActor        *dragged,
                   ClutterActor        *icon,
                   int                  x,
                   int                  y,
                   AhoghillResultsPane *pane)
{
    int tileno;

    tileno = find_tile (pane, widget);
    if (tileno == -1) {
        return;
    }
}
static void
tile_dnd_motion_cb (NbtkWidget          *widget,
                    ClutterActor        *dragged,
                    ClutterActor        *icon,
                    int                  x,
                    int                  y,
                    AhoghillResultsPane *pane)
{
    int tileno;

    tileno = find_tile (pane, widget);
    if (tileno == -1) {
        return;
    }
}

static void
tile_dnd_end_cb (NbtkWidget          *widget,
                 ClutterActor        *dragged,
                 ClutterActor        *icon,
                 int                  x,
                 int                  y,
                 AhoghillResultsPane *pane)
{
    int tileno;

    tileno = find_tile (pane, widget);
    if (tileno == -1) {
        return;
    }
}

static void
ahoghill_results_pane_init (AhoghillResultsPane *self)
{
    AhoghillResultsPanePrivate *priv = GET_PRIVATE (self);
    int i;

    self->priv = priv;

    clutter_actor_set_name (CLUTTER_ACTOR (self), "media-pane-results");
    nbtk_table_set_col_spacing (NBTK_TABLE (self), COL_SPACING);
    nbtk_table_set_row_spacing (NBTK_TABLE (self), ROW_SPACING);

    priv->title = nbtk_label_new (_("Recent"));
    clutter_actor_set_name (CLUTTER_ACTOR (priv->title), "media-pane-results-label");
    nbtk_table_add_widget_full (NBTK_TABLE (self), priv->title,
                                0, 0, 1, 6,
                                0, 0.0, 0.5);

    for (i = 0; i < TILES_PER_PAGE; i++) {
        priv->tiles[i] = g_object_new (AHOGHILL_TYPE_MEDIA_TILE, NULL);

        nbtk_widget_set_dnd_threshold (NBTK_WIDGET (priv->tiles[i]), 10);
        g_signal_connect (priv->tiles[i], "button-press-event",
                          G_CALLBACK (tile_pressed_cb), self);
        g_signal_connect (priv->tiles[i], "button-release-event",
                          G_CALLBACK (tile_released_cb), self);
        g_signal_connect (priv->tiles[i], "dnd-begin",
                          G_CALLBACK (tile_dnd_begin_cb), self);
        g_signal_connect (priv->tiles[i], "dnd-motion",
                          G_CALLBACK (tile_dnd_motion_cb), self);
        g_signal_connect (priv->tiles[i], "dnd-end",
                          G_CALLBACK (tile_dnd_end_cb), self);

        nbtk_table_add_widget_full (NBTK_TABLE (self),
                                    (NbtkWidget *) priv->tiles[i],
                                    (i / TILES_PER_ROW) + 1, i % TILES_PER_ROW,
                                    1, 1, 0, 0.5, 0.5);
        clutter_actor_hide ((ClutterActor *) priv->tiles[i]);
    }
}

static void
unref_and_free_array (GPtrArray *array)
{
    int i;

    for (i = 0; i < array->len; i++) {
        g_object_unref (array->pdata[i]);
    }

    g_ptr_array_free (array, TRUE);
}

static GPtrArray *
copy_and_ref_array (GPtrArray *original)
{
    GPtrArray *clone;
    int i;

    clone = g_ptr_array_sized_new (original->len);
    for (i = 0; i < original->len; i++) {
        g_ptr_array_add (clone, g_object_ref (original->pdata[i]));
    }

    return clone;
}

void
ahoghill_results_pane_set_results (AhoghillResultsPane *self,
                                   GPtrArray           *results)
{
    AhoghillResultsPanePrivate *priv;
    int i, count;

    priv = self->priv;

    if (priv->results) {
        unref_and_free_array (priv->results);
        priv->results = NULL;
    }

    priv->results = copy_and_ref_array (results);
    count = MIN (results->len, TILES_PER_PAGE);

    for (i = 0; i < count; i++) {
         g_object_set (priv->tiles[i],
                       "item", results->pdata[i],
                       NULL);
         clutter_actor_show ((ClutterActor *) priv->tiles[i]);
    }

    /* Clear the rest of the results */
    for (i = count; i < TILES_PER_PAGE; i++) {
        g_object_set (priv->tiles[i],
                      "item", NULL,
                      NULL);
        clutter_actor_hide ((ClutterActor *) priv->tiles[i]);
    }
}
