#include <bognor-regis/br-queue.h>

#include "ahoghill-playlist.h"
#include "ahoghill-playlist-header.h"
#include "ahoghill-queue-list.h"

enum {
    PROP_0,
};

struct _AhoghillPlaylistPrivate {
    AhoghillGridView *gridview;
    AhoghillPlaylistHeader *header;
    AhoghillQueueList *list;

    BrQueue *queue;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_PLAYLIST, AhoghillPlaylistPrivate))
G_DEFINE_TYPE (AhoghillPlaylist, ahoghill_playlist, NBTK_TYPE_TABLE);

static void
ahoghill_playlist_finalize (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_playlist_parent_class)->finalize (object);
}

static void
ahoghill_playlist_dispose (GObject *object)
{
    AhoghillPlaylist *playlist = (AhoghillPlaylist *) object;
    AhoghillPlaylistPrivate *priv = playlist->priv;

    if (priv->gridview) {
        g_object_unref (priv->gridview);
        priv->gridview = NULL;
    }

    if (priv->queue) {
        g_object_unref (priv->queue);
        priv->queue = NULL;
    }

    G_OBJECT_CLASS (ahoghill_playlist_parent_class)->dispose (object);
}

static void
ahoghill_playlist_set_property (GObject      *object,
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
ahoghill_playlist_get_property (GObject    *object,
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
ahoghill_playlist_class_init (AhoghillPlaylistClass *klass)
{
    GObjectClass *o_class = (GObjectClass *)klass;

    o_class->dispose = ahoghill_playlist_dispose;
    o_class->finalize = ahoghill_playlist_finalize;
    o_class->set_property = ahoghill_playlist_set_property;
    o_class->get_property = ahoghill_playlist_get_property;

    g_type_class_add_private (klass, sizeof (AhoghillPlaylistPrivate));
}

static void
ahoghill_playlist_init (AhoghillPlaylist *self)
{
    AhoghillPlaylistPrivate *priv;

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    priv->header = g_object_new (AHOGHILL_TYPE_PLAYLIST_HEADER, NULL);
    nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                          (ClutterActor *) priv->header, 0, 0,
                                          "x-align", 0.0,
                                          "y-align", 0.0,
                                          NULL);

    priv->list = g_object_new (AHOGHILL_TYPE_QUEUE_LIST, NULL);
    nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                          (ClutterActor *) priv->list, 1, 0,
                                          "x-align", 0.0,
                                          "y-align", 0.0,
                                          NULL);
}

AhoghillPlaylist *
ahoghill_playlist_new (AhoghillGridView *view)
{
    AhoghillPlaylist *playlist;
    AhoghillPlaylistPrivate *priv;

    playlist = g_object_new (AHOGHILL_TYPE_PLAYLIST, NULL);
    priv = playlist->priv;

    priv->gridview = g_object_ref (view);

    return playlist;
}

static void
uri_added_cb (BrQueue          *queue,
              const char       *uri,
              int               index,
              AhoghillPlaylist *playlist)
{
    AhoghillPlaylistPrivate *priv = playlist->priv;
    BklItem *item;

    item = ahoghill_grid_view_get_item (priv->gridview, uri);
    ahoghill_queue_list_add_item (priv->list, item, index);
}

static void
uri_removed_cb (BrQueue          *queue,
                const char       *uri,
                int               index,
                AhoghillPlaylist *playlist)
{
    AhoghillPlaylistPrivate *priv = playlist->priv;

    ahoghill_queue_list_remove (priv->list, index);
}

static void
now_playing_changed_cb (BrQueue          *queue,
                        const char       *uri,
                        AhoghillPlaylist *playlist)
{
    AhoghillPlaylistPrivate *priv = playlist->priv;
    BklItem *item;

    item = ahoghill_grid_view_get_item (priv->gridview, uri);
    /* Something */
}

void
ahoghill_playlist_set_queue (AhoghillPlaylist *playlist,
                             BrQueue          *queue)
{
    AhoghillPlaylistPrivate *priv = playlist->priv;

    priv->queue = g_object_ref (queue);
    g_signal_connect (queue, "uri-added",
                      G_CALLBACK (uri_added_cb), playlist);
    g_signal_connect (queue, "uri-removed",
                      G_CALLBACK (uri_removed_cb), playlist);
    g_signal_connect (queue, "now-playing-changed",
                      G_CALLBACK (now_playing_changed_cb), playlist);
}

AhoghillQueueList *
ahoghill_playlist_get_queue_list (AhoghillPlaylist *playlist)
{
    return playlist->priv->list;
}

