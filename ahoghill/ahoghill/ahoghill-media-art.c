#include <gdk-pixbuf/gdk-pixbuf.h>
#include "ahoghill-media-art.h"

enum {
    PROP_0,
    PROP_URI,
};

struct _AhoghillMediaArtPrivate {
    ClutterActor *art;
    ClutterActor *play_texture; /* This is just a clone of play_texture below */
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_MEDIA_ART, AhoghillMediaArtPrivate))
G_DEFINE_TYPE (AhoghillMediaArt, ahoghill_media_art, NBTK_TYPE_WIDGET);

static ClutterActor *play_texture = NULL;

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
ahoghill_media_art_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    AhoghillMediaArt *self = (AhoghillMediaArt *) object;
    AhoghillMediaArtPrivate *priv = self->priv;
    GError *error = NULL;
    const char *uri;
    char *path;

    switch (prop_id) {

    case PROP_URI:
        uri = g_value_get_string (value);
        if (uri == NULL) {
            guint32 data = 0;
            GError *error = NULL;

            /* FIXME: Is this the best way to clear a texture to translucent? */
            clutter_texture_set_from_rgb_data (CLUTTER_TEXTURE (priv->art),
                                               (const guchar *) &data, TRUE,
                                               1, 1, 1, 4, 0, &error);
            if (error) {
                g_warning ("Error clearing texture: %s", error->message);
                g_error_free (error);
            }
            return;
        }

        path = g_filename_from_uri (uri, NULL, NULL);

        /* FIXME: Need to scale in software so we don't get jaggies */
        clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->art),
                                       path, &error);
        if (error != NULL) {
            g_warning ("Error setting %s: %s", uri, error->message);
            g_error_free (error);
        }
        g_free (path);
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
    ClutterUnit w, h, x_off, y_off;

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

static void
ahoghill_media_art_paint (ClutterActor *actor)
{
    AhoghillMediaArtPrivate *priv = ((AhoghillMediaArt *) actor)->priv;

    CLUTTER_ACTOR_CLASS (ahoghill_media_art_parent_class)->paint (actor);
    clutter_actor_paint (priv->art);

    if (CLUTTER_ACTOR_IS_VISIBLE (priv->play_texture)) {
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
    g_object_class_install_property (o_class, PROP_URI,
                                     g_param_spec_string ("thumbnail", "", "",
                                                          "",
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

    clutter_actor_set_name (CLUTTER_ACTOR (self), "media-pane-album-art");

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

