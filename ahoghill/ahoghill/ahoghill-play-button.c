#include "ahoghill-play-button.h"

enum {
    PROP_0,
};

struct _AhoghillPlayButtonPrivate {
    int dummy;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_PLAY_BUTTON, AhoghillPlayButtonPrivate))
G_DEFINE_TYPE (AhoghillPlayButton, ahoghill_play_button, NBTK_TYPE_BUTTON);

static void
ahoghill_play_button_finalize (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_play_button_parent_class)->finalize (object);
}

static void
ahoghill_play_button_dispose (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_play_button_parent_class)->dispose (object);
}

static void
ahoghill_play_button_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
#if 0
    AhoghillPlayButton *self = (AhoghillPlayButton *) object;

    switch (prop_id) {

    default:
        break;
    }
#endif
}

static void
ahoghill_play_button_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
#if 0
    AhoghillPlayButton *self = (AhoghillPlayButton *) object;

    switch (prop_id) {

    default:
        break;
    }
#endif
}

static gboolean
ahoghill_play_button_enter (ClutterActor         *actor,
                            ClutterCrossingEvent *event)
{
    nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor), "hover");
    return FALSE;
}

static gboolean
ahoghill_play_button_leave (ClutterActor         *actor,
                            ClutterCrossingEvent *event)
{
    nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor), NULL);
    return FALSE;
}

static void
ahoghill_play_button_released (NbtkButton *button)
{
    gboolean toggled = nbtk_button_get_checked (button);

    if (toggled) {
        nbtk_widget_set_style_class_name ((NbtkWidget *) button,
                                          "Toggled");
    } else {
        nbtk_widget_set_style_class_name ((NbtkWidget *) button, NULL);
    }
}

static void
ahoghill_play_button_class_init (AhoghillPlayButtonClass *klass)
{
    GObjectClass *o_class = (GObjectClass *) klass;
    ClutterActorClass *a_class = (ClutterActorClass *) klass;
    NbtkButtonClass *b_class = (NbtkButtonClass *) klass;

    o_class->dispose = ahoghill_play_button_dispose;
    o_class->finalize = ahoghill_play_button_finalize;
    o_class->set_property = ahoghill_play_button_set_property;
    o_class->get_property = ahoghill_play_button_get_property;

    a_class->enter_event = ahoghill_play_button_enter;
    a_class->leave_event = ahoghill_play_button_leave;

    b_class->released = ahoghill_play_button_released;

    g_type_class_add_private (klass, sizeof (AhoghillPlayButtonPrivate));
}

static void
ahoghill_play_button_init (AhoghillPlayButton *self)
{
    AhoghillPlayButtonPrivate *priv;

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    nbtk_button_set_toggle_mode (NBTK_BUTTON (self), TRUE);
}

void
ahoghill_play_button_set_playing (AhoghillPlayButton *button,
                                  gboolean            playing)
{
    nbtk_button_set_checked ((NbtkButton *) button, playing);
    if (playing) {
        nbtk_widget_set_style_class_name ((NbtkWidget *) button,
                                          "Toggled");
    } else {
        nbtk_widget_set_style_class_name ((NbtkWidget *) button, NULL);
    }
}
