#include <glib/gi18n.h>
#include <bickley/bkl.h>

#include "ahoghill-playlist-header.h"
#include "ahoghill-play-button.h"

enum {
    PROP_0,
    PROP_TITLE,
};

enum {
    PLAYING,
    POS_CHANGED,
    LAST_SIGNAL
};

struct _AhoghillPlaylistHeaderPrivate {
    NbtkWidget *playlist_title;
    NbtkWidget *primary;
    NbtkWidget *secondary;

    NbtkWidget *play_button;
    guint32 play_clicked_id;

    NbtkAdjustment *audio_progress;
    guint32 pos_changed_id;
    NbtkWidget *seekbar;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_PLAYLIST_HEADER, AhoghillPlaylistHeaderPrivate))
G_DEFINE_TYPE (AhoghillPlaylistHeader, ahoghill_playlist_header, NBTK_TYPE_TABLE);
static guint32 signals[LAST_SIGNAL] = {0,};

#define HEADER_WIDTH 210
#define HEADER_HEIGHT 50
#define BUTTON_WIDTH 37
#define BUTTON_HEIGHT 37

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
    AhoghillPlaylistHeader *header = (AhoghillPlaylistHeader *) object;
    AhoghillPlaylistHeaderPrivate *priv = header->priv;
    const char *title;

    switch (prop_id) {
    case PROP_TITLE:
        title = g_value_get_string (value);
        nbtk_label_set_text (NBTK_LABEL (priv->playlist_title), title);
        break;

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

    g_object_class_install_property (o_class, PROP_TITLE,
                                     g_param_spec_string ("title", "", "", "",
                                                          G_PARAM_WRITABLE |
                                                          G_PARAM_STATIC_STRINGS));

    signals[PLAYING] = g_signal_new ("playing",
                                     G_TYPE_FROM_CLASS (klass),
                                     G_SIGNAL_RUN_FIRST |
                                     G_SIGNAL_NO_RECURSE, 0, NULL, NULL,
                                     g_cclosure_marshal_VOID__BOOLEAN,
                                     G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

static void
play_button_clicked_cb (NbtkButton             *button,
                        AhoghillPlaylistHeader *header)
{
    gboolean toggled;

    toggled = nbtk_button_get_checked (button);
    g_signal_emit (header, signals[PLAYING], 0, toggled);
}

static void
ahoghill_playlist_header_init (AhoghillPlaylistHeader *self)
{
    AhoghillPlaylistHeaderPrivate *priv;

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    clutter_actor_set_size ((ClutterActor *) self, HEADER_WIDTH, HEADER_HEIGHT);

    priv->playlist_title = nbtk_label_new ("");
    clutter_actor_set_size ((ClutterActor *) priv->playlist_title, 160, 20);
    clutter_actor_set_name ((ClutterActor *) priv->playlist_title,
                            "ahoghill-playlist-title");
    nbtk_table_add_actor (NBTK_TABLE (self),
                          (ClutterActor *) priv->playlist_title,
                          0, 0);

    priv->play_button = g_object_new (AHOGHILL_TYPE_PLAY_BUTTON, NULL);
    clutter_actor_set_size (CLUTTER_ACTOR (priv->play_button),
                            BUTTON_WIDTH, BUTTON_HEIGHT);
    priv->play_clicked_id = g_signal_connect
        (priv->play_button, "clicked",
         G_CALLBACK (play_button_clicked_cb), self);

    nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                          (ClutterActor *) priv->play_button,
                                          0, 1,
                                          "x-align", 0.0,
                                          "y-align", 0.5,
                                          NULL);
}

void
ahoghill_playlist_header_set_can_play (AhoghillPlaylistHeader *header,
                                       gboolean                can_play)
{
#if 0
    AhoghillPlaylistHeaderPrivate *priv = header->priv;

    clutter_actor_set_reactive ((ClutterActor *) priv->play_button, can_play);

    if (can_play) {
        nbtk_widget_set_style_pseudo_class (priv->play_button, NULL);
    } else {
        /* FIXME: Need an image for inactive state */
        nbtk_widget_set_style_pseudo_class (priv->play_button, "inactive");
    }
#endif
}

void
ahoghill_playlist_header_set_playing (AhoghillPlaylistHeader *header,
                                      gboolean                playing)
{
    AhoghillPlaylistHeaderPrivate *priv = header->priv;

    g_signal_handler_block (priv->play_button, priv->play_clicked_id);
    nbtk_button_set_checked ((NbtkButton *) priv->play_button, playing);
    g_signal_handler_unblock (priv->play_button, priv->play_clicked_id);
}
