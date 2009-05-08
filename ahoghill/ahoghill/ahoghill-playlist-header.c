#include "ahoghill-playlist-header.h"

enum {
    PROP_0,
};

struct _AhoghillPlaylistHeaderPrivate {
    int dummy;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_PLAYLIST_HEADER, AhoghillPlaylistHeaderPrivate))
G_DEFINE_TYPE (AhoghillPlaylistHeader, ahoghill_playlist_header, NBTK_TYPE_TABLE);

static void
ahoghill_playlist_header_finalize (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_playlist_header_parent_class)->finalize (object);
}

static void
ahoghill_playlist_header_dispose (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_playlist_header_parent_class)->dispose (object);
}

static void
ahoghill_playlist_header_set_property (GObject      *object,
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
ahoghill_playlist_header_get_property (GObject    *object,
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
ahoghill_playlist_header_class_init (AhoghillPlaylistHeaderClass *klass)
{
    GObjectClass *o_class = (GObjectClass *)klass;

    o_class->dispose = ahoghill_playlist_header_dispose;
    o_class->finalize = ahoghill_playlist_header_finalize;
    o_class->set_property = ahoghill_playlist_header_set_property;
    o_class->get_property = ahoghill_playlist_header_get_property;

    g_type_class_add_private (klass, sizeof (AhoghillPlaylistHeaderPrivate));
}

static void
ahoghill_playlist_header_init (AhoghillPlaylistHeader *self)
{
    AhoghillPlaylistHeaderPrivate *priv;
    ClutterActor *rect;
    ClutterColor red = {0xff, 0x00, 0x00, 0xff};

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    rect = clutter_rectangle_new_with_color (&red);
    clutter_actor_set_size (rect, 140, 50);
    nbtk_table_add_actor_with_properties (NBTK_TABLE (self), rect, 0, 0,
                                          "x-expand", FALSE,
                                          "y-expand", FALSE,
                                          "x-align", 0.0,
                                          "y-align", 0.0,
                                          NULL);
}

