#include <glib/gi18n.h>
#include <bickley/bkl.h>

#include "ahoghill-queue-tile.h"

enum {
    PROP_0,
};

struct _AhoghillQueueTilePrivate {
    BklItem *item;
    NbtkWidget *row1;
    NbtkWidget *row2;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_QUEUE_TILE, AhoghillQueueTilePrivate))
G_DEFINE_TYPE (AhoghillQueueTile, ahoghill_queue_tile, NBTK_TYPE_TABLE);

static void
ahoghill_queue_tile_finalize (GObject *object)
{
    AhoghillQueueTile *self = (AhoghillQueueTile *) object;

    G_OBJECT_CLASS (ahoghill_queue_tile_parent_class)->finalize (object);
}

static void
ahoghill_queue_tile_dispose (GObject *object)
{
    AhoghillQueueTile *self = (AhoghillQueueTile *) object;

    G_OBJECT_CLASS (ahoghill_queue_tile_parent_class)->dispose (object);
}

static void
ahoghill_queue_tile_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    AhoghillQueueTile *self = (AhoghillQueueTile *) object;

    switch (prop_id) {

    default:
        break;
    }
}

static void
ahoghill_queue_tile_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    AhoghillQueueTile *self = (AhoghillQueueTile *) object;

    switch (prop_id) {

    default:
        break;
    }
}

static void
ahoghill_queue_tile_class_init (AhoghillQueueTileClass *klass)
{
    GObjectClass *o_class = (GObjectClass *)klass;

    o_class->dispose = ahoghill_queue_tile_dispose;
    o_class->finalize = ahoghill_queue_tile_finalize;
    o_class->set_property = ahoghill_queue_tile_set_property;
    o_class->get_property = ahoghill_queue_tile_get_property;

    g_type_class_add_private (klass, sizeof (AhoghillQueueTilePrivate));
}

static void
ahoghill_queue_tile_init (AhoghillQueueTile *self)
{
    AhoghillQueueTilePrivate *priv;

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    priv->row1 = nbtk_label_new ("Light & Day - Reach for the Sun");
    nbtk_widget_set_style_class_name (priv->row1, "AhoghillPrimaryLabel");
    nbtk_table_add_actor (NBTK_TABLE (self),
                          (ClutterActor *) priv->row1, 0, 0);

    priv->row2 = nbtk_label_new ("The Polyphonic Spree");
    nbtk_widget_set_style_class_name (priv->row2, "AhoghillSecondaryLabel");
    nbtk_table_add_actor (NBTK_TABLE (self),
                          (ClutterActor *) priv->row2, 1, 0);
}

void
ahoghill_queue_tile_set_item (AhoghillQueueTile *tile,
                              BklItem           *item)
{
    AhoghillQueueTilePrivate *priv = tile->priv;
    GPtrArray *artists;
    char *primary = NULL, *secondary = NULL, *title;
    int w, h;

    priv->item = g_object_ref (item);
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

        title = bkl_item_video_get_title ((BklItemVideo *) item);
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

    nbtk_label_set_text (NBTK_LABEL (priv->row1), primary);
    if (secondary) {
        nbtk_label_set_text (NBTK_LABEL (priv->row2), secondary);
    }

    g_free (primary);
    g_free (secondary);
}
