#include "nbtk-fixed.h"

enum {
    PROP_0,
};

struct _NbtkFixedPrivate {
    ClutterActor *parent_group;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NBTK_TYPE_FIXED, NbtkFixedPrivate))
G_DEFINE_TYPE (NbtkFixed, nbtk_fixed, NBTK_TYPE_WIDGET);

static void
nbtk_fixed_finalize (GObject *object)
{
    g_signal_handlers_destroy (object);
    G_OBJECT_CLASS (nbtk_fixed_parent_class)->finalize (object);
}

static void
nbtk_fixed_dispose (GObject *object)
{
    G_OBJECT_CLASS (nbtk_fixed_parent_class)->dispose (object);
}

static void
nbtk_fixed_set_property (GObject      *object,
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
nbtk_fixed_get_property (GObject    *object,
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
nbtk_fixed_allocate (ClutterActor          *actor,
                     const ClutterActorBox *box,
                     gboolean               absolute_origin_changed)
{
    NbtkFixed *fixed = (NbtkFixed *) actor;
    NbtkFixedPrivate *priv = fixed->priv;
    ClutterActorClass *parent_class;
    ClutterActorBox child_box;
    NbtkPadding padding = { 0, };

    nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

    parent_class = CLUTTER_ACTOR_CLASS (nbtk_fixed_parent_class);
    parent_class->allocate (actor, box, absolute_origin_changed);

    child_box.x1 = padding.left;
    child_box.y1 = padding.top;
    child_box.x2 = box->x2 - box->x1 - padding.right;
    child_box.y2 = box->y2 - box->y1 - padding.bottom;

    clutter_actor_allocate (priv->parent_group, &child_box,
                            absolute_origin_changed);
    clutter_actor_set_clipu (priv->parent_group, 0, 0,
                             child_box.x2, child_box.y2);
}

static void
nbtk_fixed_paint (ClutterActor *actor)
{
    NbtkFixed *fixed = (NbtkFixed *) actor;
    NbtkFixedPrivate *priv = fixed->priv;

    CLUTTER_ACTOR_CLASS (nbtk_fixed_parent_class)->paint (actor);

    if (CLUTTER_ACTOR_IS_VISIBLE (priv->parent_group)) {
        clutter_actor_paint (priv->parent_group);
    }
}

static void
nbtk_fixed_class_init (NbtkFixedClass *klass)
{
    GObjectClass *o_class = (GObjectClass *) klass;
    ClutterActorClass *a_class = (ClutterActorClass *) klass;

    o_class->dispose = nbtk_fixed_dispose;
    o_class->finalize = nbtk_fixed_finalize;
    o_class->set_property = nbtk_fixed_set_property;
    o_class->get_property = nbtk_fixed_get_property;

    a_class->allocate = nbtk_fixed_allocate;
    a_class->paint = nbtk_fixed_paint;

    g_type_class_add_private (klass, sizeof (NbtkFixedPrivate));
}

static void
nbtk_fixed_init (NbtkFixed *self)
{
    NbtkFixedPrivate *priv;

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    priv->parent_group = clutter_group_new ();
    clutter_actor_set_name (priv->parent_group, "nbtk-fixed-parent-group");
    clutter_actor_set_parent (priv->parent_group, CLUTTER_ACTOR (self));
    clutter_actor_set_position (priv->parent_group, 0, 0);
    clutter_actor_show (priv->parent_group);
}

void
nbtk_fixed_add_actor (NbtkFixed    *fixed,
                      ClutterActor *actor)
{
    NbtkFixedPrivate *priv = fixed->priv;

    clutter_container_add_actor (CLUTTER_CONTAINER (priv->parent_group), actor);
}
