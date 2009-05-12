#include <glib/gi18n.h>
#include <bognor/br-queue.h>

#include "ahoghill-playlist.h"
#include "ahoghill-playlist-header.h"
#include "ahoghill-queue-list.h"

enum {
    PROP_0,
    PROP_TITLE,
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
    AhoghillPlaylist *playlist = (AhoghillPlaylist *) object;
    AhoghillPlaylistPrivate *priv = playlist->priv;

    switch (prop_id) {
    case PROP_TITLE:
        g_object_set (priv->header,
                      "title", g_value_get_string (value),
                      NULL);
        break;

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

    g_object_class_install_property (o_class, PROP_TITLE,
                                     g_param_spec_string ("title", "", "", "",
                                                          G_PARAM_WRITABLE |
                                                          G_PARAM_STATIC_STRINGS));
}

static void
header_playing_cb (AhoghillPlaylistHeader *header,
                   gboolean                playing,
                   AhoghillPlaylist       *playlist)
{
    AhoghillPlaylistPrivate *priv = playlist->priv;

    if (playing) {
        br_queue_play (priv->queue);
    } else {
        br_queue_stop (priv->queue);
    }
}

static void
ahoghill_playlist_init (AhoghillPlaylist *self)
{
    AhoghillPlaylistPrivate *priv;

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    priv->header = g_object_new (AHOGHILL_TYPE_PLAYLIST_HEADER, NULL);
    nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->header), "Top");
    ahoghill_playlist_header_set_can_play
        ((AhoghillPlaylistHeader *) priv->header, FALSE);
    g_signal_connect (priv->header, "playing",
                      G_CALLBACK (header_playing_cb), self);
    nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                          (ClutterActor *) priv->header, 0, 0,
                                          "x-expand", TRUE,
                                          "x-fill", TRUE,
                                          "y-expand", FALSE,
                                          "y-fill", FALSE,
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
ahoghill_playlist_new (AhoghillGridView *view,
                       const char       *title)
{
    AhoghillPlaylist *playlist;
    AhoghillPlaylistPrivate *priv;

    playlist = g_object_new (AHOGHILL_TYPE_PLAYLIST,
                             "title", title,
                             NULL);
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

    ahoghill_playlist_header_set_can_play
        ((AhoghillPlaylistHeader *) priv->header, TRUE);
}

static void
uri_removed_cb (BrQueue          *queue,
                const char       *uri,
                int               index,
                AhoghillPlaylist *playlist)
{
    AhoghillPlaylistPrivate *priv = playlist->priv;

    ahoghill_queue_list_remove (priv->list, index);
    /* FIXME: Set can play */
}

static void
now_playing_changed_cb (BrQueue          *queue,
                        const char       *uri,
                        int               type,
                        AhoghillPlaylist *playlist)
{
    AhoghillPlaylistPrivate *priv = playlist->priv;
    BklItem *item = NULL;

    if (uri && *uri != '\0') {
        item = ahoghill_grid_view_get_item (priv->gridview, uri);
    }

    ahoghill_playlist_header_set_item (priv->header, item);
}

static void
list_uris_reply (BrQueue  *queue,
                 char    **uris,
                 GError   *error,
                 gpointer  userdata)
{
    AhoghillPlaylist *playlist = (AhoghillPlaylist *) userdata;
    AhoghillPlaylistPrivate *priv = playlist->priv;
    int i;

    if (error != NULL) {
        g_warning ("(%s) Error getting uris from Bognor-Regis: %s",
                   G_STRLOC, error->message);
    }

    for (i = 0; uris[i]; i++) {
        BklItem *item = ahoghill_grid_view_get_item (priv->gridview, uris[i]);

        g_print ("uri: %s - %p\n", uris[i], item);
        ahoghill_queue_list_add_item (priv->list, item, i);
    }

    if (uris && uris[0] != NULL) {
        ahoghill_playlist_header_set_can_play
            ((AhoghillPlaylistHeader *) priv->header, TRUE);
    }
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

    br_queue_list_uris (queue, list_uris_reply, playlist);
}

AhoghillQueueList *
ahoghill_playlist_get_queue_list (AhoghillPlaylist *playlist)
{
    return playlist->priv->list;
}

