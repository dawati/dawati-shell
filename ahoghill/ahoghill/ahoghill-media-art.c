#include <gdk-pixbuf/gdk-pixbuf.h>
#include <bickley/bkl-item-image.h>

#include "ahoghill-media-art.h"

enum {
    PROP_0,
    PROP_ITEM,
};

struct _AhoghillMediaArtPrivate {
    ClutterActor *art;
    double rotation;
    ClutterActor *play_texture; /* This is just a clone of play_texture below */
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_MEDIA_ART, AhoghillMediaArtPrivate))
G_DEFINE_TYPE (AhoghillMediaArt, ahoghill_media_art, NBTK_TYPE_WIDGET);

static ClutterActor *play_texture = NULL;

static GdkPixbuf *generic_album = NULL;
static GdkPixbuf *generic_image = NULL;
static GdkPixbuf *generic_video = NULL;

static void
ahoghill_media_art_finalize (GObject *object)
{
    g_signal_handlers_destroy (object);
    G_OBJECT_CLASS (ahoghill_media_art_parent_class)->finalize (object);
}

static void
ahoghill_media_art_dispose (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_media_art_parent_class)->dispose (object);
}

static void
clear_texture (AhoghillMediaArt *art)
{
    AhoghillMediaArtPrivate *priv = art->priv;
    GError *error = NULL;
    guint32 data = 0;

    clutter_texture_set_from_rgb_data (CLUTTER_TEXTURE (priv->art),
                                       (const guchar *) &data, TRUE,
                                       1, 1, 1, 4, 0, &error);
    if (error) {
        g_warning ("Error clearing texture: %s", error->message);
        g_error_free (error);
    }
}

static void
set_texture_from_pixbuf (ClutterActor *texture,
                         GdkPixbuf    *thumbnail)
{
    GError *error = NULL;
    gboolean has_alpha;

    has_alpha = gdk_pixbuf_get_has_alpha (thumbnail);
    clutter_texture_set_from_rgb_data (CLUTTER_TEXTURE (texture),
                                       gdk_pixbuf_get_pixels (thumbnail),
                                       has_alpha,
                                       gdk_pixbuf_get_width (thumbnail),
                                       gdk_pixbuf_get_height (thumbnail),
                                       gdk_pixbuf_get_rowstride (thumbnail),
                                       has_alpha ? 4 : 3, 0,
                                       &error);
    if (error != NULL) {
        g_warning ("Error setting %s", error->message);
        g_error_free (error);
    }
}

static void
use_default_texture (AhoghillMediaArt *art,
                     BklItem          *item)
{
    AhoghillMediaArtPrivate *priv = art->priv;
    GdkPixbuf *thumbnail;
    GError *error = NULL;

    switch (bkl_item_get_item_type (item)) {
    case BKL_ITEM_TYPE_AUDIO:
        if (G_UNLIKELY (generic_album == NULL)) {
            generic_album = gdk_pixbuf_new_from_file
                (PKG_DATADIR "/theme/media-panel/hrn-generic-album.png",
                 &error);

            if (generic_album == NULL) {
                g_warning ("Error loading Generic album image: %s",
                           error->message);
                g_error_free (error);
                clear_texture (art);
                return;
            }
        }

        thumbnail = generic_album;
        break;

    case BKL_ITEM_TYPE_IMAGE:
        if (G_UNLIKELY (generic_image == NULL)) {
            generic_image = gdk_pixbuf_new_from_file
                (PKG_DATADIR "/theme/media-panel/hrn-generic-image.png",
                 &error);

            if (generic_image == NULL) {
                g_warning ("Error loading Generic image image: %s",
                           error->message);
                g_error_free (error);
                clear_texture (art);
                return;
            }
        }

        thumbnail = generic_image;
        break;

    case BKL_ITEM_TYPE_VIDEO:
        if (G_UNLIKELY (generic_video == NULL)) {
            generic_video = gdk_pixbuf_new_from_file
                (PKG_DATADIR "/theme/media-panel/hrn-generic-video.png",
                 &error);

            if (generic_video == NULL) {
                g_warning ("Error loading Generic video image: %s",
                           error->message);
                g_error_free (error);
                clear_texture (art);
                return;
            }
        }

        thumbnail = generic_video;
        break;

    default:
        clear_texture (art);
        return;
    }

    set_texture_from_pixbuf (priv->art, thumbnail);
}

static void
ahoghill_media_art_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
    AhoghillMediaArt *self = (AhoghillMediaArt *) object;
    AhoghillMediaArtPrivate *priv = self->priv;
    GError *error = NULL;
    BklItem *item;
    GdkPixbuf *thumbnail;
    const char *uri = NULL;
    char *path;

    switch (prop_id) {

    case PROP_ITEM:
        item = g_value_get_object (value);
        if (item) {
            uri = bkl_item_extended_get_thumbnail ((BklItemExtended *) item);
        }

        if (uri == NULL) {
            if (item) {
                use_default_texture (self, item);
            } else {
                clear_texture (self);
            }
            return;
        }

        path = g_filename_from_uri (uri, NULL, NULL);

        thumbnail = gdk_pixbuf_new_from_file (path, &error);
        if (error != NULL) {
            g_warning ("Error loading pixbuf %s: %s", uri, error->message);
            g_error_free (error);
            g_free (path);
            return;
        }

        if (bkl_item_get_item_type (item) == BKL_ITEM_TYPE_IMAGE) {
            BklItemImage *im = (BklItemImage *) item;
            const char *orient;

            orient = bkl_item_image_get_orientation (im);
            if (orient) {
                GdkPixbuf *pb = NULL;

                if (g_str_equal (orient, "right - top")) {
                    pb = gdk_pixbuf_rotate_simple (thumbnail,
                                                   GDK_PIXBUF_ROTATE_CLOCKWISE);
                } else if (g_str_equal (orient, "left - top")) {
                    pb = gdk_pixbuf_rotate_simple (thumbnail,
                                                   GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
                }

                if (pb) {
                    g_object_unref (thumbnail);
                    thumbnail = pb;
                }
            }
        }

        set_texture_from_pixbuf (priv->art, thumbnail);
        g_free (path);

        g_object_unref (thumbnail);
        break;

    default:
        break;
    }
}

static void
ahoghill_media_art_get_property (GObject    *object,
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
ahoghill_media_art_allocate (ClutterActor          *actor,
                             const ClutterActorBox *box,
                             gboolean               absolute_origin_changed)
{
    AhoghillMediaArtPrivate *priv = ((AhoghillMediaArt *) actor)->priv;
    ClutterActorClass *parent_class;
    ClutterActorBox child_box, play_box;
    NbtkPadding padding = { 0, };
    gfloat w, h, x_off, y_off;

    nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

    parent_class = CLUTTER_ACTOR_CLASS (ahoghill_media_art_parent_class);
    parent_class->allocate (actor, box, absolute_origin_changed);

    child_box.x1 = 10;
    child_box.y1 = 7;
    child_box.x2 = box->x2 - box->x1 - 12;
    child_box.y2 = box->y2 - box->y1 - 12;

    clutter_actor_allocate (priv->art, &child_box, absolute_origin_changed);

    w = clutter_actor_get_widthu (priv->play_texture);
    h = clutter_actor_get_heightu (priv->play_texture);

    x_off = ((child_box.x2 - child_box.x1) - w) / 2;
    y_off = ((child_box.y2 - child_box.y1) - h) / 2;

    play_box.x1 = child_box.x1 + x_off;
    play_box.y1 = child_box.y1 + y_off;
    play_box.x2 = play_box.x1 + w;
    play_box.y2 = play_box.y1 + h;

    clutter_actor_allocate (priv->play_texture, &play_box, absolute_origin_changed);
}

/* Taken from PengeMagicTexture */
static void
paint_art (ClutterActor *actor)
{
    ClutterActorBox box;
    CoglHandle *tex;
    int bw, bh;
    int aw, ah;
    float v;
    float tx1, tx2, ty1, ty2;
    ClutterColor col = { 0xff, 0xff, 0xff, 0xff };

    clutter_actor_get_allocation_box (actor, &box);
    tex = clutter_texture_get_cogl_texture ((ClutterTexture *) actor);

    bw = cogl_texture_get_width (tex); /* base texture width */
    bh = cogl_texture_get_height (tex); /* base texture height */

    aw = CLUTTER_UNITS_TO_INT (box.x2 - box.x1); /* allocation width */
    ah = CLUTTER_UNITS_TO_INT (box.y2 - box.y1); /* allocation height */

    /* no comment */
    if ((float)bw/bh < (float)aw/ah) {
        /* fit width */
        v = (((float)ah * bw) / ((float)aw * bh)) / 2;
        tx1 = 0;
        tx2 = 1;
        ty1 = (0.5 - v);
        ty2 = (0.5 + v);
    } else {
        /* fit height */
        v = (((float)aw * bh) / ((float)ah * bw)) / 2;
        tx1 = (0.5 - v);
        tx2 = (0.5 + v);
        ty1 = 0;
        ty2 = 1;
    }

    col.alpha = clutter_actor_get_paint_opacity (actor);
    cogl_set_source_color4ub (col.red, col.green, col.blue, col.alpha);
    cogl_rectangle (CLUTTER_UNITS_TO_INT (box.x1),
                    CLUTTER_UNITS_TO_INT (box.y1),
                    CLUTTER_UNITS_TO_INT (box.x2),
                    CLUTTER_UNITS_TO_INT (box.y2));
    cogl_set_source_texture (tex);
    cogl_rectangle_with_texture_coords (CLUTTER_UNITS_TO_INT (box.x1),
                                        CLUTTER_UNITS_TO_INT (box.y1),
                                        CLUTTER_UNITS_TO_INT (box.x2),
                                        CLUTTER_UNITS_TO_INT (box.y2),
                                        tx1, ty1,
                                        tx2, ty2);
}

static void
ahoghill_media_art_paint (ClutterActor *actor)
{
    AhoghillMediaArtPrivate *priv = ((AhoghillMediaArt *) actor)->priv;

    CLUTTER_ACTOR_CLASS (ahoghill_media_art_parent_class)->paint (actor);

    paint_art (priv->art);

    if (CLUTTER_ACTOR_IS_MAPPED (priv->play_texture)) {
        clutter_actor_paint (priv->play_texture);
    }
}

static gboolean
ahoghill_media_art_enter (ClutterActor         *actor,
                          ClutterCrossingEvent *event)
{
    AhoghillMediaArt *art = (AhoghillMediaArt *) actor;
    AhoghillMediaArtPrivate *priv = art->priv;

    nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor), "hover");

    clutter_actor_animate (priv->play_texture, CLUTTER_EASE_OUT_EXPO, 2000,
                           "opacity", 0xff,
                           NULL);
    return FALSE;
}

static gboolean
ahoghill_media_art_leave (ClutterActor         *actor,
                          ClutterCrossingEvent *event)
{
    AhoghillMediaArt *art = (AhoghillMediaArt *) actor;
    AhoghillMediaArtPrivate *priv = art->priv;

    nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor), NULL);

    clutter_actor_animate (priv->play_texture, CLUTTER_EASE_OUT_EXPO, 2000,
                           "opacity", 0x00,
                           NULL);
    return FALSE;
}

static void
ahoghill_media_art_class_init (AhoghillMediaArtClass *klass)
{
    GObjectClass *o_class = (GObjectClass *)klass;
    ClutterActorClass *a_class = (ClutterActorClass *) klass;

    o_class->dispose = ahoghill_media_art_dispose;
    o_class->finalize = ahoghill_media_art_finalize;
    o_class->set_property = ahoghill_media_art_set_property;
    o_class->get_property = ahoghill_media_art_get_property;

    a_class->paint = ahoghill_media_art_paint;
    a_class->allocate = ahoghill_media_art_allocate;
    a_class->enter_event = ahoghill_media_art_enter;
    a_class->leave_event = ahoghill_media_art_leave;

    g_type_class_add_private (klass, sizeof (AhoghillMediaArtPrivate));
    g_object_class_install_property (o_class, PROP_ITEM,
                                     g_param_spec_object ("item", "", "",
                                                          BKL_TYPE_ITEM,
                                                          G_PARAM_WRITABLE |
                                                          G_PARAM_STATIC_STRINGS));
}

static void
ahoghill_media_art_init (AhoghillMediaArt *self)
{
    AhoghillMediaArtPrivate *priv = GET_PRIVATE (self);
    NbtkPadding padding = { 0, };

    self->priv = priv;

    nbtk_widget_get_padding (NBTK_WIDGET (self), &padding);

    clutter_actor_set_reactive ((ClutterActor *) self, TRUE);

    priv->art = clutter_texture_new ();
    clutter_actor_set_parent ((ClutterActor *) priv->art,
                              (ClutterActor *) self);
    clutter_actor_show ((ClutterActor *) priv->art);

    if (G_UNLIKELY (play_texture == NULL)) {
        GError *error = NULL;

        play_texture = clutter_texture_new_from_file (PKG_DATADIR "/theme/media-panel/play_hover.png",
                                                      &error);
        if (play_texture == NULL) {
            g_warning ("Error loading play texture: %s", error->message);
            g_error_free (error);
        }
        clutter_actor_show (play_texture);
    }

    if (play_texture) {
        priv->play_texture = clutter_clone_new (play_texture);
        clutter_actor_set_parent (priv->play_texture,
                                  (ClutterActor *) self);
        clutter_actor_set_opacity (priv->play_texture, 0x00);
    }
}

