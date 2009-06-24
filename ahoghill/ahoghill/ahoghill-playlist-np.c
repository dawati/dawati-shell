#include <glib.h>
#include <glib/gi18n.h>

#include <bickley/bkl-item-audio.h>
#include <bickley/bkl-item-image.h>
#include <bickley/bkl-item-video.h>

#include "ahoghill-playlist-np.h"

enum {
    PROP_0,
};

enum {
    POS_CHANGED,
    LAST_SIGNAL
};

struct _AhoghillPlaylistNpPrivate {
    NbtkWidget *primary;
    NbtkWidget *secondary;

    NbtkAdjustment *audio_progress;
    guint32 pos_changed_id;
    NbtkWidget *seekbar;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_PLAYLIST_NP, AhoghillPlaylistNpPrivate))
G_DEFINE_TYPE (AhoghillPlaylistNp, ahoghill_playlist_np, NBTK_TYPE_TABLE);

static guint32 signals[LAST_SIGNAL] = {0, };

static void
ahoghill_playlist_np_finalize (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_playlist_np_parent_class)->finalize (object);
}

static void
ahoghill_playlist_np_dispose (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_playlist_np_parent_class)->dispose (object);
}

static void
ahoghill_playlist_np_class_init (AhoghillPlaylistNpClass *klass)
{
    GObjectClass *o_class = (GObjectClass *)klass;

    o_class->dispose = ahoghill_playlist_np_dispose;
    o_class->finalize = ahoghill_playlist_np_finalize;

    g_type_class_add_private (klass, sizeof (AhoghillPlaylistNpPrivate));

    signals[POS_CHANGED] = g_signal_new ("position-changed",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_FIRST |
                                         G_SIGNAL_NO_RECURSE, 0, NULL, NULL,
                                         g_cclosure_marshal_VOID__DOUBLE,
                                         G_TYPE_NONE, 1, G_TYPE_DOUBLE);
}

static void
seek_position_changed (NbtkAdjustment     *adjustment,
                       GParamSpec         *pspec,
                       AhoghillPlaylistNp *np)
{
    double position = nbtk_adjustment_get_value (adjustment);

    g_signal_emit (np, signals[POS_CHANGED], 0, position);
}

static void
ahoghill_playlist_np_init (AhoghillPlaylistNp *self)
{
    AhoghillPlaylistNpPrivate *priv;

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    priv->primary = nbtk_label_new ("");
    nbtk_widget_set_style_class_name (priv->primary, "AhoghillPrimaryLabel");
    nbtk_table_add_actor (NBTK_TABLE (self),
                          (ClutterActor *) priv->primary,
                          0, 0);

    priv->secondary = nbtk_label_new ("");
    nbtk_widget_set_style_class_name (priv->secondary,
                                      "AhoghillSecondaryLabel");
    nbtk_table_add_actor (NBTK_TABLE (self),
                          (ClutterActor *) priv->secondary,
                          1, 0);

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
ahoghill_playlist_np_set_item (AhoghillPlaylistNp *now_playing,
                               BklItem            *item)
{
    AhoghillPlaylistNpPrivate *priv = now_playing->priv;
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
ahoghill_playlist_np_set_position (AhoghillPlaylistNp *now_playing,
                                   double              position)
{
    AhoghillPlaylistNpPrivate *priv = now_playing->priv;

    g_signal_handler_block (priv->audio_progress, priv->pos_changed_id);
    nbtk_adjustment_set_value (priv->audio_progress, position);
    g_signal_handler_unblock (priv->audio_progress, priv->pos_changed_id);
}

