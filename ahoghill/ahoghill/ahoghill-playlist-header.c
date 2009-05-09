#include "ahoghill-playlist-header.h"

enum {
    PROP_0,
};

struct _AhoghillPlaylistHeaderPrivate {
    NbtkWidget *playlist_title;
    NbtkWidget *primary;
    NbtkWidget *secondary;
    NbtkWidget *seekbar;
    NbtkWidget *play_button;
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

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    priv->playlist_title = nbtk_label_new ("Local");
    clutter_actor_set_name ((ClutterActor *) priv->playlist_title,
                            "ahoghill-playlist-title");
    nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                          (ClutterActor *) priv->playlist_title,
                                          0, 0,
                                          "y-expand", FALSE,
                                          "y-fill", TRUE,
                                          "x-align", 0.0,
                                          NULL);

    priv->play_button = nbtk_button_new ();
    nbtk_button_set_toggle_mode (NBTK_BUTTON (priv->play_button), TRUE);
    nbtk_widget_set_style_class_name (priv->play_button,
                                      "AhoghillPlayButton");
    clutter_actor_set_size (CLUTTER_ACTOR (priv->play_button), 28, 31);

    nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                          (ClutterActor *) priv->play_button,
                                          0, 1,
                                          "x-expand", FALSE,
                                          "y-expand", FALSE,
                                          NULL);

    priv->primary = nbtk_label_new ("Wall-E");
    clutter_actor_set_name ((ClutterActor *) priv->primary,
                            "ahoghill-playlist-header-primary");
    nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                          (ClutterActor *) priv->primary,
                                          1, 0,
                                          "col-span", 2,
                                          "y-expand", FALSE,
                                          "x-align", 0.0,
                                          NULL);

    priv->secondary = nbtk_label_new ("Pixar");
    clutter_actor_set_name ((ClutterActor *) priv->secondary,
                            "ahoghill-playlist-header-secondary");
    nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                          (ClutterActor *) priv->secondary,
                                          2, 0,
                                          "col-span", 2,
                                          "y-expand", FALSE,
                                          "x-align", 0.0,
                                          NULL);
}

