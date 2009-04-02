#include <bickley/bkl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <nbtk/nbtk-label.h>

#include "ahoghill-media-art.h"
#include "ahoghill-media-tile.h"

enum {
    PROP_0,
    PROP_ITEM,
};

struct _AhoghillMediaTilePrivate {
    GdkPixbuf *thumb;
    BklItem *item;

    NbtkWidget *icon;
    NbtkWidget *artist;
    NbtkWidget *title;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_MEDIA_TILE, AhoghillMediaTilePrivate))
G_DEFINE_TYPE (AhoghillMediaTile, ahoghill_media_tile, NBTK_TYPE_TABLE);

#define SPACING 5

#define THUMBNAIL_WIDTH 120
#define THUMBNAIL_HEIGHT 117

static void
ahoghill_media_tile_finalize (GObject *object)
{
    g_signal_handlers_destroy (object);
    G_OBJECT_CLASS (ahoghill_media_tile_parent_class)->finalize (object);
}

static void
ahoghill_media_tile_dispose (GObject *object)
{
    AhoghillMediaTile *self = (AhoghillMediaTile *) object;
    AhoghillMediaTilePrivate *priv = self->priv;

    if (priv->thumb) {
        g_object_unref (priv->thumb);
        priv->thumb = NULL;
    }

    if (priv->item) {
        g_object_unref (priv->item);
        priv->item = NULL;
    }

    G_OBJECT_CLASS (ahoghill_media_tile_parent_class)->dispose (object);
}

static void
update_tile (AhoghillMediaTile *self)
{
    AhoghillMediaTilePrivate *priv = self->priv;
    GPtrArray *artists;
    char *title = NULL, *artist = NULL;
    guint width, height, season, episode, year;

    switch (bkl_item_get_item_type (priv->item)) {
    case BKL_ITEM_TYPE_AUDIO:
        title = g_strdup (bkl_item_audio_get_title ((BklItemAudio *) priv->item));
        artists = bkl_item_audio_get_artists ((BklItemAudio *) priv->item);

        /* FIXME: make a string from the array */
        artist = g_strdup (artists->pdata[0]);
        break;

    case BKL_ITEM_TYPE_IMAGE:
        title = g_strdup (bkl_item_image_get_title ((BklItemImage *) priv->item));
        width = bkl_item_image_get_width ((BklItemImage *) priv->item);
        height = bkl_item_image_get_height ((BklItemImage *) priv->item);
        artist = g_strdup_printf ("%ux%u", width, height);
        break;

    case BKL_ITEM_TYPE_VIDEO:
        title = g_strdup (bkl_item_video_get_title ((BklItemVideo *) priv->item));
        year = bkl_item_video_get_year ((BklItemVideo *) priv->item);
        if (year < 1900) {
            season = bkl_item_video_get_season ((BklItemVideo *) priv->item);
            episode = bkl_item_video_get_episode ((BklItemVideo *) priv->item);

            artist = g_strdup_printf ("Season %u, Episode %u", season, episode);
        } else {
            artist = g_strdup_printf ("(%u)", year);
        }
        break;

    default:
        break;
    }

    if (title == NULL) {
        title = g_path_get_basename (bkl_item_get_uri (priv->item));
    }
    nbtk_label_set_text (NBTK_LABEL (priv->title), title);
    nbtk_label_set_text (NBTK_LABEL (priv->artist), artist ? artist : bkl_item_get_mimetype (priv->item));

    g_object_set (priv->icon,
                  "thumbnail", bkl_item_extended_get_thumbnail ((BklItemExtended *) priv->item),
                  NULL);

    g_free (title);
    g_free (artist);
}

static void
clear_tile (AhoghillMediaTile *tile)
{
    AhoghillMediaTilePrivate *priv = tile->priv;

    nbtk_label_set_text (NBTK_LABEL (priv->title), "");
    nbtk_label_set_text (NBTK_LABEL (priv->artist), "");

    g_object_set (priv->icon,
                  "thumbnail", NULL,
                  NULL);
}

static void
ahoghill_media_tile_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
    AhoghillMediaTile *self = (AhoghillMediaTile *) object;
    AhoghillMediaTilePrivate *priv = self->priv;

    switch (prop_id) {

    case PROP_ITEM:
        if (priv->item) {
            g_object_unref (priv->item);
        }

        priv->item = g_value_dup_object (value);
        if (priv->item) {
            update_tile (self);
        } else {
            clear_tile (self);
        }
        break;

    default:
        break;
    }
}

static void
ahoghill_media_tile_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
    switch (prop_id) {

    case PROP_ITEM:
    default:
        break;
    }
}

static void
ahoghill_media_tile_class_init (AhoghillMediaTileClass *klass)
{
    GObjectClass *o_class = (GObjectClass *) klass;

    o_class->dispose = ahoghill_media_tile_dispose;
    o_class->finalize = ahoghill_media_tile_finalize;
    o_class->set_property = ahoghill_media_tile_set_property;
    o_class->get_property = ahoghill_media_tile_get_property;

    g_type_class_add_private (klass, sizeof (AhoghillMediaTilePrivate));

    g_object_class_install_property (o_class, PROP_ITEM,
                                     g_param_spec_object ("item", "", "",
                                                          BKL_TYPE_ITEM,
                                                          G_PARAM_WRITABLE |
                                                          G_PARAM_STATIC_STRINGS));
}

static void
ahoghill_media_tile_init (AhoghillMediaTile *self)
{
    AhoghillMediaTilePrivate *priv = GET_PRIVATE (self);

    self->priv = priv;

    clutter_actor_set_size (CLUTTER_ACTOR (self), 120, 151);

    priv->icon = g_object_new (AHOGHILL_TYPE_MEDIA_ART, NULL);
    clutter_actor_set_size (CLUTTER_ACTOR (priv->icon),
                            THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT);
    clutter_actor_show (CLUTTER_ACTOR (priv->icon));

    nbtk_table_add_widget_full (NBTK_TABLE (self), priv->icon,
                                0, 0, 1, 1,
                                NBTK_X_EXPAND | NBTK_Y_EXPAND | NBTK_X_FILL | NBTK_Y_FILL,
                                0.5, 0.5);

    priv->title = nbtk_label_new ("");
    clutter_actor_set_name (CLUTTER_ACTOR (priv->title),
                            "media-tile-title-label");
    nbtk_table_add_widget_full (NBTK_TABLE (self), priv->title,
                                1, 0, 1, 1, 0, 0.0, 0.5);

    priv->artist = nbtk_label_new ("");
    clutter_actor_set_name (CLUTTER_ACTOR (priv->artist),
                            "media-tile-artist-label");
    nbtk_table_add_widget_full (NBTK_TABLE (self), priv->artist,
                                2, 0, 1, 1, 0, 0.0, 0.5);
}
