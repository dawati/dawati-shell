#include <bickley/bkl.h>

#include "ahoghill-grid-view.h"
#include "ahoghill-results-pane.h"
#include "ahoghill-search-pane.h"

enum {
    PROP_0,
};

typedef struct _Source {
    KozoDB *db;

    GPtrArray *items;
    GHashTable *uri_to_item;
} Source;

struct _AhoghillGridViewPrivate {
    ClutterActor *search_pane;
    ClutterActor *results_pane;
    ClutterActor *playqueues_pane;

    BklWatcher *watcher;
    GPtrArray *dbs;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_GRID_VIEW, AhoghillGridViewPrivate))
G_DEFINE_TYPE (AhoghillGridView, ahoghill_grid_view, NBTK_TYPE_TABLE);

static void
ahoghill_grid_view_finalize (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_grid_view_parent_class)->finalize (object);
}

static void
ahoghill_grid_view_dispose (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_grid_view_parent_class)->dispose (object);
}

static void
ahoghill_grid_view_set_property (GObject      *object,
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
ahoghill_grid_view_get_property (GObject    *object,
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
ahoghill_grid_view_class_init (AhoghillGridViewClass *klass)
{
    GObjectClass *o_class = (GObjectClass *) klass;

    o_class->dispose = ahoghill_grid_view_dispose;
    o_class->finalize = ahoghill_grid_view_finalize;
    o_class->set_property = ahoghill_grid_view_set_property;
    o_class->get_property = ahoghill_grid_view_get_property;

    g_type_class_add_private (klass, sizeof (AhoghillGridViewPrivate));
}

static Source *
create_source (BklDBSource *s)
{
    Source *source;
    GError *error = NULL;

    source = g_new0 (Source, 1);
    source->db = bkl_db_get_for_path (s->db_uri, s->name, &error);
    if (error != NULL) {
        g_warning ("Error loading %s (%s): %s", s->name, s->db_uri,
                   error->message);
        g_error_free (error);
        g_free (source);
        return NULL;
    }

    source->items = bkl_db_get_items (source->db, FALSE, &error);
    if (source->items == NULL) {
        g_warning ("Error getting items: %s", error->message);
        kozo_db_unref (source->db);
        g_free (source);
        return NULL;
    }

    return source;
}

static GPtrArray *
test_results (Source *source)
{
    GPtrArray *results;
    int i = 0;

    if (sources->items->len == 0)
      return NULL;

    if (sources->items->len < 12)
    {
      results = g_ptr_array_sized_new (source->items->len);

      while (i < source->items->len)
      {
        BklItem *item = source->items->pdata[i]

        if (bkl_item_extended_get_thumbnail ((BklItemExtended *) item)) {
            g_ptr_array_add (results, g_object_ref (item));
        }
      }

      return results;
    }

    results = g_ptr_array_sized_new (12);
    while (i < 200 && results->len < 12) {
        BklItem *item = source->items->pdata[rand () % source->items->len];

        if (bkl_item_extended_get_thumbnail ((BklItemExtended *) item)) {
            g_ptr_array_add (results, g_object_ref (item));
        }

        i++;
    }

    return results;
}

static gboolean
init_bickley (gpointer data)
{
    AhoghillGridView *view = (AhoghillGridView *) data;
    AhoghillGridViewPrivate *priv = view->priv;
    GPtrArray *results = NULL;
    GList *sources, *s;
    GError *error = NULL;
    int i;

    priv->watcher = g_object_new (BKL_TYPE_WATCHER, NULL);
    sources = bkl_watcher_get_sources (priv->watcher, &error);
    if (error != NULL) {
        g_warning ("Error getting sources\n");
    }

    priv->dbs = g_ptr_array_sized_new (g_list_length (sources));
    for (s = sources; s; s = s->next) {
        Source *source = create_source ((BklDBSource *) s->data);

        g_ptr_array_add (priv->dbs, source);
    }

    /* Get 12 random entries with thumbnails for testing purposes */
    i = 0;
    while (results == NULL && i < priv->dbs->len) {
        Source *source = priv->dbs->pdata[i];

        /* Find local database */
        if (!g_str_equal (kozo_db_get_name (source->db), "local-media")) {
            i++;
            continue;
        }

        results = test_results (priv->dbs->pdata[i]);
    }

    if (results) {
        ahoghill_results_pane_set_results
            (AHOGHILL_RESULTS_PANE (priv->results_pane), results);
    }

    return FALSE;
}

static void
ahoghill_grid_view_init (AhoghillGridView *self)
{
    NbtkTable *table = (NbtkTable *) self;
    AhoghillGridViewPrivate *priv = GET_PRIVATE (self);

    ClutterColor blue = { 0x00, 0x00, 0xff, 0xff };

    bkl_init ();

    clutter_actor_set_name (CLUTTER_ACTOR (self), "media-pane-vbox");
    clutter_actor_set_size (CLUTTER_ACTOR (self), 1024, 500);

    self->priv = priv;
    priv->search_pane = g_object_new (AHOGHILL_TYPE_SEARCH_PANE, NULL);
    clutter_actor_set_size (priv->search_pane, 1024, 50);
    nbtk_table_add_actor_full (table, priv->search_pane,
                               0, 0, 1, 4, NBTK_X_EXPAND, 0.0, 0.0);

    priv->results_pane = g_object_new (AHOGHILL_TYPE_RESULTS_PANE, NULL);
    clutter_actor_set_size (priv->results_pane, 800, 400);
    nbtk_table_add_actor_full (table, priv->results_pane,
                               1, 0, 1, 3,
                               NBTK_X_FILL | NBTK_Y_FILL | NBTK_X_EXPAND | NBTK_Y_EXPAND,
                               0.0, 0.0);

    /* priv->playqueues_pane = g_object_new (AHOGHILL_TYPE_PLAYQUEUES_PANE, */
    /*                                        NULL); */
    priv->playqueues_pane = clutter_rectangle_new_with_color (&blue);
    clutter_actor_set_size (priv->playqueues_pane, 150, 400);
    nbtk_table_add_actor_full (table, priv->playqueues_pane,
                               1, 3, 1, 1, NBTK_Y_FILL, 0.0, 0.0);

    nbtk_table_set_row_spacing (table, 8);
    nbtk_table_set_col_spacing (table, 8);

    /* Init Bickley lazily */
    g_idle_add (init_bickley, self);
}

