#include <bickley/bkl-item.h>

#include "ahoghill-queue-list.h"
#include "ahoghill-queue-tile.h"

enum {
    PROP_0,
};

struct _AhoghillQueueListPrivate {
    NbtkWidget *viewport;
    ClutterActor *children;

    GPtrArray *items;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_QUEUE_LIST, AhoghillQueueListPrivate))
G_DEFINE_TYPE (AhoghillQueueList, ahoghill_queue_list, NBTK_TYPE_SCROLL_VIEW);

static void
ahoghill_queue_list_finalize (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_queue_list_parent_class)->finalize (object);
}

static void
ahoghill_queue_list_dispose (GObject *object)
{
    G_OBJECT_CLASS (ahoghill_queue_list_parent_class)->dispose (object);
}

static void
ahoghill_queue_list_set_property (GObject      *object,
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
ahoghill_queue_list_get_property (GObject    *object,
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
ahoghill_queue_list_class_init (AhoghillQueueListClass *klass)
{
    GObjectClass *o_class = (GObjectClass *)klass;

    o_class->dispose = ahoghill_queue_list_dispose;
    o_class->finalize = ahoghill_queue_list_finalize;
    o_class->set_property = ahoghill_queue_list_set_property;
    o_class->get_property = ahoghill_queue_list_get_property;

    g_type_class_add_private (klass, sizeof (AhoghillQueueListPrivate));
}

static void
ahoghill_queue_list_init (AhoghillQueueList *self)
{
    AhoghillQueueListPrivate *priv;

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    priv->viewport = nbtk_viewport_new ();

    clutter_actor_set_position ((ClutterActor *) priv->viewport, 5, 5);
    clutter_container_add_actor (CLUTTER_CONTAINER (self),
                                 CLUTTER_ACTOR (priv->viewport));

    priv->children = clutter_group_new ();
    clutter_container_add (CLUTTER_CONTAINER (priv->viewport),
                           priv->children, NULL);

    priv->items = g_ptr_array_new ();
}

/* Why does GLib not have this function */
static void
ptr_array_insert (GPtrArray *array,
                  int        index,
                  gpointer   data)
{
    int i;

    /* This will cause the extra space to be allocated */
    g_ptr_array_add (array, data);

    for (i = array->len - 2; i > index; i--) {
        array->pdata[i] = array->pdata[i - 1];
    }

    array->pdata[index] = data;
}

void
ahoghill_queue_list_add_item (AhoghillQueueList *list,
                              BklItem           *item,
                              int                index)
{
    AhoghillQueueListPrivate *priv = list->priv;
    AhoghillQueueTile *tile;
    guint height;
    int i;

    tile = g_object_new (AHOGHILL_TYPE_QUEUE_TILE, NULL);
    if (item) {
        ahoghill_queue_tile_set_item (tile, item);
    }

    /* Make sure the index is within the bounds of the list */
    index = CLAMP (index, 0, priv->items->len);
    if (index == priv->items->len) {
        g_ptr_array_add (priv->items, tile);
    } else {
        /* Move the rest of the items */
        for (i = index; i < priv->items->len; i++) {
            int x, y;

            clutter_actor_get_position (CLUTTER_ACTOR (priv->items->pdata[i]),
                                        &x, &y);
            clutter_actor_get_size (CLUTTER_ACTOR (priv->items->pdata[i]),
                                    NULL, &height);
            clutter_actor_set_position (CLUTTER_ACTOR (priv->items->pdata[i]),
                                        x, y + height);
        }

        ptr_array_insert (priv->items, index, tile);
    }

    clutter_container_add_actor (CLUTTER_CONTAINER (priv->children),
                                 CLUTTER_ACTOR (tile));

    clutter_actor_get_size (CLUTTER_ACTOR (tile), NULL, &height);
    clutter_actor_set_position (CLUTTER_ACTOR (tile), 0,
                                index * height);
    clutter_actor_show ((ClutterActor *) tile);
}

void
ahoghill_queue_list_remove (AhoghillQueueList *list,
                            int                index)
{
    AhoghillQueueListPrivate *priv = list->priv;
    AhoghillQueueTile *tile;
    guint height = 0;
    int i;

    tile = priv->items->pdata[index];
    if (tile) {
        clutter_actor_get_size ((ClutterActor *) tile, NULL, &height);
        clutter_container_remove_actor (CLUTTER_CONTAINER (priv->children),
                                        (ClutterActor *) tile);
    }

    for (i = index + 1; i < priv->items->len; i++) {
        ClutterActor *actor = (ClutterActor *) priv->items->pdata[i];
        int x, y;

        /* FIXME: Fancy animation? */
        clutter_actor_get_position (actor, &x, &y);
        clutter_actor_set_position (actor, x, y - height);
    }

    g_ptr_array_remove_index (priv->items, index);
}
