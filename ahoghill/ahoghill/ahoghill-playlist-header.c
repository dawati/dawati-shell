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
    signals[POS_CHANGED] = g_signal_new ("position-changed",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_FIRST |
                                         G_SIGNAL_NO_RECURSE, 0, NULL, NULL,
                                         g_cclosure_marshal_VOID__DOUBLE,
                                         G_TYPE_NONE, 1, G_TYPE_DOUBLE);
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
seek_position_changed (NbtkAdjustment         *adjustment,
                       GParamSpec             *pspec,
                       AhoghillPlaylistHeader *header)
{
    double position = nbtk_adjustment_get_value (adjustment);

    g_signal_emit (header, signals[POS_CHANGED], 0, position);
}

static void
ahoghill_playlist_header_init (AhoghillPlaylistHeader *self)
{
    AhoghillPlaylistHeaderPrivate *priv;

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    clutter_actor_set_size ((ClutterActor *) self, 210, -1);

    priv->playlist_title = nbtk_label_new ("");
    clutter_actor_set_name ((ClutterActor *) priv->playlist_title,
                            "ahoghill-playlist-title");
    nbtk_table_add_actor (NBTK_TABLE (self),
                          (ClutterActor *) priv->playlist_title,
                          0, 0);

    priv->play_button = g_object_new (AHOGHILL_TYPE_PLAY_BUTTON, NULL);
    clutter_actor_set_size (CLUTTER_ACTOR (priv->play_button), 28, 31);
    priv->play_clicked_id = g_signal_connect
        (priv->play_button, "clicked",
         G_CALLBACK (play_button_clicked_cb), self);

    nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                          (ClutterActor *) priv->play_button,
                                          0, 1,
                                          "x-expand", TRUE,
                                          "x-align", 0.0,
                                          NULL);

    priv->primary = nbtk_label_new ("");
    clutter_actor_set_name ((ClutterActor *) priv->primary,
                            "ahoghill-playlist-header-primary");
    nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                          (ClutterActor *) priv->primary,
                                          1, 0,
                                          "col-span", 2,
                                          "y-expand", FALSE,
                                          "x-align", 0.0,
                                          NULL);

    priv->secondary = nbtk_label_new ("");
    clutter_actor_set_name ((ClutterActor *) priv->secondary,
                            "ahoghill-playlist-header-secondary");
    nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                          (ClutterActor *) priv->secondary,
                                          2, 0,
                                          "col-span", 2,
                                          "y-expand", FALSE,
                                          "x-align", 0.0,
                                          NULL);

    priv->audio_progress = nbtk_adjustment_new (0.0, 0.0, 1.0, 0.0, 0.0, 0.0);
    priv->pos_changed_id = g_signal_connect
        (priv->audio_progress, "notify::value",
         G_CALLBACK (seek_position_changed), self);
    priv->seekbar = nbtk_scroll_bar_new (priv->audio_progress);
    nbtk_widget_set_style_class_name (priv->seekbar, "AhoghillProgressBar");
    clutter_actor_set_size ((ClutterActor *) priv->seekbar, -1, 20);
    clutter_actor_set_reactive ((ClutterActor *) priv->seekbar, FALSE);

    nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                          (ClutterActor *) priv->seekbar,
                                          3, 0,
                                          "col-span", 2,
                                          "x-align", 0.0,
                                          NULL);
}

void
ahoghill_playlist_header_set_item (AhoghillPlaylistHeader *header,
                                   BklItem                *item)
{
    AhoghillPlaylistHeaderPrivate *priv = header->priv;
    GPtrArray *artists;
    char *primary = NULL, *secondary = NULL, *title;
    int w, h;

    if (item == NULL) {
        clutter_actor_set_reactive ((ClutterActor *) priv->seekbar, FALSE);

        /* FIXME: Fade out text? */
        nbtk_label_set_text ((NbtkLabel *) priv->primary, "");
        nbtk_label_set_text ((NbtkLabel *) priv->secondary, "");
        return;
    }

    clutter_actor_set_reactive ((ClutterActor *) priv->seekbar, TRUE);

    switch (bkl_item_get_item_type (item)) {
    case BKL_ITEM_TYPE_AUDIO:
        primary = g_strdup (bkl_item_audio_get_title ((BklItemAudio *) item));
        artists = bkl_item_audio_get_artists ((BklItemAudio *) item);

        if (artists) {
            /* FIXME: Use more than the first artist? */
            secondary = g_strdup (artists->pdata[0]);
        }
        break;

    case BKL_ITEM_TYPE_IMAGE:
        primary = g_strdup (bkl_item_image_get_title ((BklItemImage *) item));
        w = bkl_item_image_get_width ((BklItemImage *) item);
        h = bkl_item_image_get_height ((BklItemImage *) item);

        secondary = g_strdup_printf ("%dx%d", w, h);
        break;

    case BKL_ITEM_TYPE_VIDEO:
        primary = g_strdup (bkl_item_video_get_series_name ((BklItemVideo *) item));

        title = (char *) bkl_item_video_get_title ((BklItemVideo *) item);
        if (title) {
            secondary = g_strdup (title);
        } else {
            int s, e, year;

            s = bkl_item_video_get_season ((BklItemVideo *) item);
            e = bkl_item_video_get_episode ((BklItemVideo *) item);
            year = bkl_item_video_get_year ((BklItemVideo *) item);

            if (s > 0 && e > 0) {
                secondary = g_strdup_printf (_("Season %d Episode %d"), s, e);
            } else if (s > 0) {
                secondary = g_strdup_printf (_("Season %d"), s);
            } else if (e > 0) {
                secondary = g_strdup_printf (_("Episode %d"), e);
            } else if (year > 1900 && year < 2200) {
                secondary = g_strdup_printf (_("Year %d"), year);
            } else {
                secondary = NULL;
            }
        }
        break;

    default:
        break;
    }

    if (primary == NULL) {
        primary = g_path_get_basename (bkl_item_get_uri (item));
    }

    nbtk_label_set_text (NBTK_LABEL (priv->primary), primary);
    if (secondary) {
        nbtk_label_set_text (NBTK_LABEL (priv->secondary), secondary);
    }

    g_free (primary);
    g_free (secondary);
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
ahoghill_playlist_header_set_position (AhoghillPlaylistHeader *header,
                                       double                  position)
{
    AhoghillPlaylistHeaderPrivate *priv = header->priv;

    g_signal_handler_block (priv->audio_progress, priv->pos_changed_id);
    nbtk_adjustment_set_value (priv->audio_progress, position);
    g_signal_handler_unblock (priv->audio_progress, priv->pos_changed_id);
}

void
ahoghill_playlist_header_set_playing (AhoghillPlaylistHeader *header,
                                      gboolean                playing)
{
    AhoghillPlaylistHeaderPrivate *priv = header->priv;

    g_signal_handler_block (priv->play_button, priv->play_clicked_id);

    g_print ("Setting checked: %s\n", playing ? "True" : "False");
    nbtk_button_set_checked (priv->play_button, playing);
    g_signal_handler_unblock (priv->play_button, priv->play_clicked_id);
}
