#include <config.h>

#include <glib.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <bickley/bkl.h>
#include <bickley/bkl-source-client.h>
#include <bickley/bkl-source-manager-client.h>

#include <bognor/br-queue.h>

#include <moblin-panel/mpl-entry.h>

#include "ahoghill-grid-view.h"
#include "ahoghill-playlist.h"
#include "ahoghill-queue-list.h"
#include "ahoghill-results-model.h"
#include "ahoghill-results-pane.h"
#include "ahoghill-search-pane.h"
#include "ahoghill-playlist-placeholder.h"

enum {
    PROP_0,
};

enum {
    DISMISS,
    LAST_SIGNAL
};

typedef struct _Source {
    BklSourceClient *source;
    BklDB *db;

    GSequence *index;
    guint32 update_id;

    GHashTable *uri_to_item;
} Source;

struct _AhoghillGridViewPrivate {
    ClutterActor *search_pane;
    ClutterActor *results_pane;
    ClutterActor *playqueues_pane;

    AhoghillResultsModel *model;
    GtkRecentManager *recent_manager;

    GPtrArray *dbs;

    BrQueue *local_queue;

    BklSourceManagerClient *source_manager;

    guint32 search_id;

    guint32 source_count;
    guint32 source_replies; /* Keep track of how many sources have replied */
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_GRID_VIEW, AhoghillGridViewPrivate))
G_DEFINE_TYPE (AhoghillGridView, ahoghill_grid_view, NBTK_TYPE_TABLE);
static guint32 signals[LAST_SIGNAL] = {0, };

/* Adjustable length of time between typing a letter and search taking place */
#define SEARCH_TIMEOUT 50

#define UPDATE_INDEX_TIMEOUT 5 /* seconds */
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

    signals[DISMISS] = g_signal_new ("dismiss",
                                     G_TYPE_FROM_CLASS (klass),
                                     G_SIGNAL_RUN_FIRST |
                                     G_SIGNAL_NO_RECURSE, 0, NULL, NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE, 0);
}

static void
uri_added_cb (BklSourceClient *client,
              const char      *uri,
              Source          *source)
{
    /* We don't care about items being added */
    /* FIXME: We should recheck if this item should be added to the results */
}

static void
uri_deleted_cb (BklSourceClient *client,
                const char      *uri,
                Source          *source)
{
    BklItem *item;

    item = g_hash_table_lookup (source->uri_to_item, uri);
    if (item == NULL) {
        return;
    }

    g_hash_table_remove (source->uri_to_item, uri);
    g_object_unref (item);
}

static void
uri_changed_cb (BklSourceClient *client,
                const char      *uri,
                Source          *source)
{
    /* FIXME: Bickley needs a way to update BklItems in place */
}

static int
compare_words (gconstpointer a,
               gconstpointer b,
               gpointer      userdata)
{
    return strcmp (a, b);
}

static void
index_changed_cb (BklSourceClient *client,
                  const char     **added,
                  const char     **removed,
                  Source          *source)
{
    GSequenceIter *iter;
    int i;

    if (source->index == NULL) {
        /* If we've not loaded the index yet, then we don't care
           about any changes made */
        return;
    }

    /* Remove the words from the index first */
    for (i = 0; removed[i]; i++) {
        char *word;

        iter = g_sequence_search (source->index, (char *) removed[i],
                                  (GCompareDataFunc) compare_words, NULL);
        word = g_sequence_get (iter);
        if (word && g_str_equal (word, removed[i])) {
            g_sequence_remove (iter);
        }
    }

    /* Add the new words */
    for (i = 0; added[i]; i++) {
        g_sequence_insert_sorted (source->index, g_strdup (added[i]),
                                  (GCompareDataFunc) compare_words, NULL);
    }

    /* FIXME: Should update the search here */
}

static Source *
create_source (BklSourceClient *s)
{
    Source *source;

    source = g_new0 (Source, 1);

    g_signal_connect (s, "uri-added",
                      G_CALLBACK (uri_added_cb), source);
    g_signal_connect (s, "uri-deleted",
                      G_CALLBACK (uri_deleted_cb), source);
    g_signal_connect (s, "uri-changed",
                      G_CALLBACK (uri_changed_cb), source);
    g_signal_connect (s, "index-changed",
                      G_CALLBACK (index_changed_cb), source);

    source->source = s;
    source->db = bkl_source_client_get_db (s);
    if (source->db == NULL) {
        g_warning ("%s: Error getting DB for %s",
                   G_STRLOC, bkl_source_client_get_path (s));

        g_object_unref (source->source);
        g_free (source);
        return NULL;
    }

    source->uri_to_item = g_hash_table_new (g_str_hash, g_str_equal);

    return source;
}

static void
destroy_source (Source *source)
{
    if (source->index) {
        g_sequence_free (source->index);
    }

    g_hash_table_destroy (source->uri_to_item);

    g_object_unref (source->source);

    g_free (source);
}

static BklItem *
find_item (AhoghillGridView *view,
           const char       *uri,
           Source          **source)
{
    AhoghillGridViewPrivate *priv = view->priv;
    int i;

    for (i = 0; i < priv->dbs->len; i++) {
        Source *s = priv->dbs->pdata[i];
        BklItem *item;
        GError *error = NULL;

        item = g_hash_table_lookup (s->uri_to_item, uri);
        if (item) {
            *source = s;
            return item;
        }

        /* Check the db */
        item = bkl_db_get_item (s->db, uri, &error);
        if (item) {
            *source = s;

            /* Put it into the item cache */
            g_hash_table_insert (s->uri_to_item,
                                 (char *) bkl_item_get_uri (item), item);
            return item;
        }
    }

    return NULL;
}

struct _ForeachData {
    Source *source;
    AhoghillResultsModel *model;
};

static gboolean
build_example_results (BklDB      *db,
                       const char *key,
                       BklItem    *item,
                       gpointer    userdata)
{
    struct _ForeachData *data = (struct _ForeachData *) userdata;
    BklItem *cached_item;
    gboolean got_all = FALSE;

    cached_item = g_hash_table_lookup (data->source->uri_to_item, key);
    if (cached_item == NULL) {
        g_hash_table_insert (data->source->uri_to_item,
                             (char *)bkl_item_get_uri (item),
                             g_object_ref (item));
        cached_item = item;
    }

    ahoghill_results_model_add_item (data->model, data->source->source,
                                     cached_item);

    got_all = (ahoghill_results_model_get_count (data->model) == 6);

    return !got_all;
}

static void
generate_example_results (AhoghillGridView *view)
{
    AhoghillGridViewPrivate *priv = view->priv;
    struct _ForeachData data;
    int i;

    for (i = 0; i < priv->dbs->len; i++) {
        Source *source = priv->dbs->pdata[i];

        data.source = source;
        data.model = priv->model;

        bkl_db_foreach (source->db, build_example_results, &data);
        if (ahoghill_results_model_get_count (priv->model) == 6) {
            return;
        }
    }
}

static int
sort_recent_list (gconstpointer a,
                  gconstpointer b)
{
    GtkRecentInfo *info_a, *info_b;
    time_t time_a, time_b;

    info_a = (GtkRecentInfo *) a;
    info_b = (GtkRecentInfo *) b;

    time_a = gtk_recent_info_get_modified (info_a);
    time_b = gtk_recent_info_get_modified (info_b);

    /* we want to sort in reverse order */
    return time_b - time_a;
}

static void
set_recent_items (AhoghillGridView *view)
{
    AhoghillGridViewPrivate *priv;
    GList *recent_items, *r;
    gboolean added_something = FALSE;

    priv = view->priv;

    /* Freeze and clear the old results before adding anything new */
    ahoghill_results_model_freeze (priv->model);
    ahoghill_results_model_clear (priv->model);

    recent_items = gtk_recent_manager_get_items (priv->recent_manager);
    if (recent_items) {
        GList *results = NULL;

        /* We collect the relevant item mimetypes from the recent items */
        for (r = recent_items; r; r = r->next) {
            GtkRecentInfo *info = r->data;
            const char *mimetype;

            mimetype = gtk_recent_info_get_mime_type (info);
            if (g_str_has_prefix (mimetype, "audio/") == FALSE &&
                g_str_has_prefix (mimetype, "video/") == FALSE &&
                g_str_has_prefix (mimetype, "image/") == FALSE) {
                gtk_recent_info_unref (info);
                continue;
            }

            results = g_list_prepend (results, info);
        }

        results = g_list_sort (results, (GCompareFunc) sort_recent_list);
        /* Now they're sorted we add them to the model */
        for (r = results; r; r = r->next) {
            Source *source;
            GtkRecentInfo *info = (GtkRecentInfo *) r->data;
            const char *uri;
            BklItem *item;

            uri = gtk_recent_info_get_uri (info);

            item = find_item (view, uri, &source);
            if (item == NULL) {
                gtk_recent_info_unref (info);
                continue;
            }

            added_something = TRUE;
            ahoghill_results_model_add_item (priv->model, source->source, item);

            gtk_recent_info_unref (info);
        }
        g_list_free (results);

        g_list_free (recent_items);
    }

    if (!added_something) {
        generate_example_results (view);
    } else {
        g_object_set (priv->results_pane,
                      "title", _("Recently played"),
                      NULL);
    }

    ahoghill_results_pane_show_example_media
        ((AhoghillResultsPane *) priv->results_pane, !added_something);

    /* And thaw the results */
    ahoghill_results_model_thaw (priv->model);
}

static void
set_result_items (AhoghillGridView *view,
                  GList            *results)
{
    AhoghillGridViewPrivate *priv = view->priv;
    GList *r;

    /* Set the results pane to the first page */
    ahoghill_results_pane_set_page
        ((AhoghillResultsPane *) priv->results_pane, 0);

    ahoghill_results_pane_show_example_media
        ((AhoghillResultsPane *) priv->results_pane, FALSE);

    g_object_set (priv->results_pane,
                  "title", _("Search results"),
                  NULL);

    /* Freeze and clear the old results before adding anything new */
    ahoghill_results_model_freeze (priv->model);
    ahoghill_results_model_clear (priv->model);

    for (r = results; r; r = r->next) {
        char *uri = r->data;
        BklItem *item;
        Source *source;

        item = find_item (view, uri, &source);
        if (item == NULL) {
            g_warning ("Cannot find item for %s", uri);
            continue;
        }

        ahoghill_results_model_add_item (priv->model, source->source, item);
    }

    ahoghill_results_model_thaw (priv->model);
}

static void
source_ready_cb (BklSourceClient  *client,
                 AhoghillGridView *view)
{
    AhoghillGridViewPrivate *priv = view->priv;
    Source *source;

    priv->source_replies++;

    source = create_source (client);
    if (source) {
        g_ptr_array_add (priv->dbs, source);
    }

    /* Once we've got all the replies from the sources,
       set up the queues and the recent items */
    if (priv->source_count == priv->source_replies) {
        set_recent_items (view);

        /* Set the local queue to the playlist */
        /* FIXME: Generate multiple playlists once more than
           local queue works */
        ahoghill_playlist_set_queue ((AhoghillPlaylist *) priv->playqueues_pane,
                                     priv->local_queue);
    }
}

static void
source_manager_added (BklSourceManagerClient *source_manager,
                      const char             *path,
                      AhoghillGridView       *view)
{
    BklSourceClient *client;

    g_print ("Adding new source: %s\n", path);
    client = bkl_source_client_new (path);
    g_signal_connect (client, "ready", G_CALLBACK (source_ready_cb), view);
}

static void
source_manager_removed (BklSourceManagerClient *source_manager,
                        const char             *path,
                        AhoghillGridView       *view)
{
    AhoghillGridViewPrivate *priv = view->priv;
    Source *source = NULL;
    int i;

    g_print ("Removing source: %s\n", path);
    for (i = 0; i < priv->dbs->len; i++) {
        Source *s = priv->dbs->pdata[i];

        if (g_str_equal (path, bkl_source_client_get_path (s->source))) {
            source = s;
            g_ptr_array_remove_index (priv->dbs, i);
            break;
        }
    }

    if (source == NULL) {
        return;
    }

    ahoghill_results_model_remove_source_items (priv->model, source->source);
    destroy_source (source);
}

static void
get_sources_reply (BklSourceManagerClient *source_manager,
                   GList                  *sources,
                   GError                 *error,
                   gpointer                data)
{
    AhoghillGridView *view = (AhoghillGridView *) data;
    AhoghillGridViewPrivate *priv = view->priv;
    BklSourceClient *client;
    GList *s;

    if (error != NULL) {
        g_warning ("Error getting sources: %s", error->message);
    }

    /* Local source first even if there was an error getting the rest */
    client = bkl_source_client_new (BKL_LOCAL_SOURCE_PATH);
    g_signal_connect (client, "ready", G_CALLBACK (source_ready_cb), view);

    for (s = sources; s; s = s->next) {
        BklSourceClient *client;

        client = bkl_source_client_new (s->data);
        g_signal_connect (client, "ready", G_CALLBACK (source_ready_cb), view);

        priv->source_count++;
    }
}

static void
source_manager_ready (BklSourceManagerClient *source_manager,
                      AhoghillGridView       *view)
{
    AhoghillGridViewPrivate *priv = view->priv;

    priv->source_replies = 0;
    priv->source_count = 1; /* Set to 1 to count the local */

    bkl_source_manager_client_get_sources (source_manager,
                                           get_sources_reply,
                                           view);
}

static void
init_bickley (gpointer data)
{
    AhoghillGridView *view = (AhoghillGridView *) data;
    AhoghillGridViewPrivate *priv = view->priv;

    priv->source_manager = g_object_new (BKL_TYPE_SOURCE_MANAGER_CLIENT, NULL);
    g_signal_connect (priv->source_manager, "ready",
                      G_CALLBACK (source_manager_ready), view);
    g_signal_connect (priv->source_manager, "source-added",
                      G_CALLBACK (source_manager_added), view);
    g_signal_connect (priv->source_manager, "source-removed",
                      G_CALLBACK (source_manager_removed), view);

    priv->dbs = g_ptr_array_new ();
}

static void
init_bognor (AhoghillGridView *grid)
{
    AhoghillGridViewPrivate *priv = grid->priv;

    priv->local_queue = g_object_new (BR_TYPE_QUEUE,
                                      "object-path", BR_LOCAL_QUEUE_PATH,
                                      NULL);
}

static gboolean
finish_init (gpointer data)
{
    AhoghillGridView *grid = data;

    init_bognor (grid);
    init_bickley (grid);

    return FALSE;
}

struct _Bartowski {
    int word_count;
    GList *chuck;
};

static void
flash_intersect (gpointer key,
                 gpointer value,
                 gpointer userdata)
{
    struct _Bartowski *bart = (struct _Bartowski *) userdata;
    int count = GPOINTER_TO_INT (value);

    /* We generate the intersection of the sets by including only
       uris that have a word count equal to the number of sets */
    if (count == bart->word_count) {
        bart->chuck = g_list_prepend (bart->chuck, g_strdup (key));
    }
}

static GList *
find_intersect (GPtrArray *sets)
{
    struct _Bartowski *bart;
    GHashTable *set;
    int i;

    set = g_hash_table_new (g_str_hash, g_str_equal);
    for (i = 0; i < sets->len; i++) {
        GList *results, *r;

        results = sets->pdata[i];

        /* Put each result into the hashtable */
        for (r = results; r; r = r->next) {
            gpointer key, value;

            /* If the result is already in the hash table
               increment its word count */
            if (g_hash_table_lookup_extended (set, r->data, &key, &value)) {
                guint count = GPOINTER_TO_INT (value);
                g_hash_table_insert (set, r->data, GINT_TO_POINTER (count + 1));
            } else {
                g_hash_table_insert (set, r->data, GINT_TO_POINTER (1));
            }
        }
    }

    bart = g_newa (struct _Bartowski, 1);
    bart->word_count = sets->len;
    bart->chuck = NULL;

    g_hash_table_foreach (set, flash_intersect, bart);
    g_hash_table_destroy (set);

    return bart->chuck;
}

static void
steal_keys (gpointer key,
            gpointer value,
            gpointer userdata)
{
    GList **results = (GList **) userdata;

    *results = g_list_append (*results, g_strdup ((char *) key));
}

static GPtrArray *
search_for_uris (Source    *source,
                 GPtrArray *possible_words)
{
    GPtrArray *possible_uris;
    int i;

    possible_uris = g_ptr_array_sized_new (possible_words->len);
    for (i = 0; i < possible_words->len; i++) {
        GPtrArray *words = possible_words->pdata[i];
        GList *results = NULL;
        GHashTable *set;
        int j;

        set = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
        for (j = 0; j < words->len; j++) {
            char *word = words->pdata[j];
            GList *r, *rr;
            GError *error = NULL;

            r = kozo_db_index_lookup (source->db->db, word, &error);
            if (error != NULL) {
                g_warning ("Error getting results for %s: %s", word,
                           error->message);
                g_error_free (error);
                r = NULL;
            }

            /* Put uris into the hashtable so we can get only unique
               entries */
            for (rr = r; rr; rr = rr->next) {
                if (g_hash_table_lookup (set, rr->data) == NULL) {
                    g_hash_table_insert (set, rr->data, rr->data);
                } else {
                    g_free (rr->data); /* Free data that doesn't go in @set */
                }
            }

            /* The contents of the list are freed either above, or when
               @set is destroyed */
            g_list_free (r);
        }

        /* Can't use g_hash_table_get_keys as it doesn't copy the data */
        g_hash_table_foreach (set, steal_keys, &results);
        g_hash_table_destroy (set);

        g_ptr_array_add (possible_uris, results);
    }

    return possible_uris;
}

static GPtrArray *
search_index_for_words (Source     *source,
                        const char *text)
{
    char *search_text;
    char **search_terms;
    GPtrArray *words;
    int k;

    search_text = g_ascii_strup (text, -1);
    search_terms = g_strsplit (search_text, " ", -1);

    /* Search each index for possible substrings in search_words */
    words = g_ptr_array_new ();
    for (k = 0; search_terms[k]; k++) {
        GPtrArray *possible;
        GSequenceIter *iter;

        /* If @text ends in a space, then g_strsplit will insert a blank
           string in @search_terms */
        if (*(search_terms[k]) == '\0') {
            continue;
        }

        possible = g_ptr_array_new ();

        g_ptr_array_add (words, possible);

        if (G_UNLIKELY (source->index == NULL)) {
            GTimer *timer = g_timer_new ();
            /* Index is a sorted array of strings */
            source->index = bkl_db_get_index_words (source->db);

            g_print ("Time taken to load index: %.3f\n",
                     g_timer_elapsed (timer, NULL));
            g_timer_destroy (timer);
        }

        iter = g_sequence_get_begin_iter (source->index);
        while (!g_sequence_iter_is_end (iter)) {
            char *word = g_sequence_get (iter);

            if (word) {
                char *new_word = g_filename_from_uri (word, NULL, NULL);
                /* The word may be en encoded uri, so we need to unencode it
                   before searching */
                if (new_word == NULL) {
                    new_word = g_strdup (word);
                }

                if (strstr (new_word, search_terms[k])) {
                    g_ptr_array_add (possible, word);
                }

                g_free (new_word);
            }

            iter = g_sequence_iter_next (iter);
        }
    }

    g_strfreev (search_terms);

    return words;
}

static gboolean
do_search_cb (gpointer data)
{
    AhoghillGridView *view = (AhoghillGridView *) data;
    AhoghillGridViewPrivate *priv = view->priv;
    NbtkWidget *entry;
    const char *text;

    entry = ahoghill_search_pane_get_entry (AHOGHILL_SEARCH_PANE (priv->search_pane));
    text = mpl_entry_get_text ((MplEntry *) entry);

    if (text == NULL || *text == 0) {
        g_print ("Setting recent items\n");
        set_recent_items (view);
    } else {
        GList *results = NULL, *u;
        int i;

        /* FIXME: We should cache results for each word in the sentence,
           and only recalculate the sets for words that have changed */
        for (i = 0; i < priv->dbs->len; i++) {
            Source *source = priv->dbs->pdata[i];
            GPtrArray *index_words;
            GPtrArray *possible_results;
            GList *r;
            int i;

            /* @index_words is a GPtrArray containing GPtrArrays
               that contain all the possible words for each word in @text */
            index_words = search_index_for_words (source, text);

            /* @possible_results is a GPtrArray containing GLists
               that contain all the possible uris for each array of words
               in @index_words */
            possible_results = search_for_uris (source, index_words);

            /* Now we intersect all the results in @possible_results to get
               the real results */
            r = find_intersect (possible_results);
            results = g_list_concat (results, r);

            for (i = 0; i < index_words->len; i++) {
                GPtrArray *words = index_words->pdata[i];

                /* Don't need to free the words, they're owned by the
                   index */
                g_ptr_array_free (words, TRUE);
            }
            g_ptr_array_free (index_words, TRUE);

            for (i = 0; i < possible_results->len; i++) {
                GList *words = possible_results->pdata[i];
                GList *w;

                for (w = words; w; w = w->next) {
                    g_free (w->data);
                }
                g_list_free (words);
            }
            g_ptr_array_free (possible_results, TRUE);
        }

        set_result_items (view, results);
        for (u = results; u; u = u->next) {
            g_free (u->data);
        }
        g_list_free (results);
    }

    priv->search_id = 0;
    return FALSE;
}

static void
search_text_changed (MplEntry         *entry,
                     AhoghillGridView *view)
{
    AhoghillGridViewPrivate *priv;

    priv = view->priv;
    if (priv->search_id != 0) {
        /* Reset timeout */
        g_source_remove (priv->search_id);
    }

    priv->search_id = g_timeout_add (SEARCH_TIMEOUT, do_search_cb, view);
}

static void
item_clicked_cb (AhoghillResultsPane *pane,
                 BklItem             *item,
                 AhoghillGridView    *grid)
{
#if 1
    AhoghillGridViewPrivate *priv = grid->priv;
    GError *error = NULL;

    br_queue_play_uri (priv->local_queue, bkl_item_get_uri (item),
                       bkl_item_get_mimetype (item));
    if (error != NULL) {
        g_warning ("%s: Error playing %s: %s", G_STRLOC,
                   bkl_item_get_uri (item),
                   error->message);
        g_error_free (error);
    }

    g_signal_emit (grid, signals[DISMISS], 0);
#else
    char *argv[3] = { "/usr/bin/hornsey", NULL, NULL };
    GError *error = NULL;

    argv[1] = (char *) bkl_item_get_uri (item);

    g_print ("Spawning hornsey for %s\n", argv[1]);
    g_spawn_async (NULL, argv, NULL, 0, NULL, NULL, NULL, &error);
    if (error != NULL) {
        g_warning ("Error launching Hornsey: %s", error->message);
        g_error_free (error);
    }
#endif
}

static void
ahoghill_grid_view_init (AhoghillGridView *self)
{
    NbtkTable *table = (NbtkTable *) self;
    AhoghillGridViewPrivate *priv = GET_PRIVATE (self);
    NbtkWidget *entry;

    bkl_init ();

    priv->model = g_object_new (AHOGHILL_TYPE_RESULTS_MODEL, NULL);

    clutter_actor_set_size (CLUTTER_ACTOR (self), 1024, 500);

    self->priv = priv;

    priv->recent_manager = gtk_recent_manager_get_default ();

    priv->search_pane = g_object_new (AHOGHILL_TYPE_SEARCH_PANE, NULL);
    clutter_actor_set_size (priv->search_pane, 1024, 50);
    nbtk_table_add_actor_with_properties (table, priv->search_pane,
                                          0, 0,
                                          "col-span", 4,
                                          "x-fill", FALSE,
                                          "y-expand", FALSE,
                                          "y-fill", FALSE,
                                          "x-align", 0.0,
                                          "y-align", 0.0,
                                          NULL);

    entry = ahoghill_search_pane_get_entry (AHOGHILL_SEARCH_PANE (priv->search_pane));
    g_signal_connect (entry, "text-changed",
                      G_CALLBACK (search_text_changed), self);

    priv->results_pane = (ClutterActor *) ahoghill_results_pane_new (priv->model);
    g_object_set (priv->results_pane,
                  "title", _("Recently played"),
                  NULL);
    /* clutter_actor_set_size (priv->results_pane, 750, 400); */
    nbtk_table_add_actor_with_properties (table, priv->results_pane,
                                          1, 0,
                                          "row-span", 1,
                                          "col-span", 3,
                                          "x-align", 0.0,
                                          "y-align", 0.0,
                                          NULL);
    g_signal_connect (priv->results_pane, "item-clicked",
                      G_CALLBACK (item_clicked_cb), self);

    priv->playqueues_pane = (ClutterActor *) ahoghill_playlist_new (self,
                                                                    _("Local"));
    clutter_actor_set_size (priv->playqueues_pane, 238, 400);
    nbtk_table_add_actor_with_properties (table, priv->playqueues_pane,
                                          1, 3,
                                          "x-expand", FALSE,
                                          "x-fill", FALSE,
                                          "y-expand", FALSE,
                                          "x-align", 0.0,
                                          "y-align", 0.0,
                                          NULL);

    /* Init Bickley and Bognor lazily */
    g_idle_add (finish_init, self);
}

void
ahoghill_grid_view_clear (AhoghillGridView *view)
{
    AhoghillGridViewPrivate *priv = view->priv;
    NbtkWidget *entry;

    entry = ahoghill_search_pane_get_entry (AHOGHILL_SEARCH_PANE (priv->search_pane));
    mpl_entry_set_text (MPL_ENTRY (entry), "");
}

void
ahoghill_grid_view_focus (AhoghillGridView *view)
{
    AhoghillGridViewPrivate *priv = view->priv;
    NbtkWidget *entry;

    entry = ahoghill_search_pane_get_entry (AHOGHILL_SEARCH_PANE (priv->search_pane));
    clutter_actor_grab_key_focus (CLUTTER_ACTOR (entry));
}

void
ahoghill_grid_view_unfocus (AhoghillGridView *view)
{
    AhoghillGridViewPrivate *priv = view->priv;
    ClutterStage *stage = CLUTTER_STAGE (clutter_stage_get_default ());
    ClutterActor *entry;
    ClutterActor *current_focus;

    /*
     * If the entry is focused, than release the focus.
     */
    entry = CLUTTER_ACTOR (
     ahoghill_search_pane_get_entry (AHOGHILL_SEARCH_PANE (priv->search_pane)));

    current_focus = clutter_stage_get_key_focus (stage);

    if (current_focus == entry)
      clutter_stage_set_key_focus (stage, NULL);
}

BklItem *
ahoghill_grid_view_get_item (AhoghillGridView *view,
                             const char       *uri)
{
    Source *source;
    BklItem *item;

    item = find_item (view, uri, &source);

    return item;
}
