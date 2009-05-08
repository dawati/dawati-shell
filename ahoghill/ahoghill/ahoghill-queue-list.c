#include <bickley/bkl-item.h>

#include "ahoghill-queue-list.h"
#include "ahoghill-queue-tile.h"

enum {
    PROP_0,
};

struct _AhoghillQueueListPrivate {
    NbtkWidget *grid;

    GPtrArray *items;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_QUEUE_LIST, AhoghillQueueListPrivate))
G_DEFINE_TYPE (AhoghillQueueList, ahoghill_queue_list, NBTK_TYPE_SCROLL_VIEW);

static void
ahoghill_queue_list_finalize (GObject *object)
{
    AhoghillQueueList *self = (AhoghillQueueList *) object;

    G_OBJECT_CLASS (ahoghill_queue_list_parent_class)->finalize (object);
}

static void
ahoghill_queue_list_dispose (GObject *object)
{
    AhoghillQueueList *self = (AhoghillQueueList *) object;

    G_OBJECT_CLASS (ahoghill_queue_list_parent_class)->dispose (object);
}

static void
ahoghill_queue_list_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    AhoghillQueueList *self = (AhoghillQueueList *) object;

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
    AhoghillQueueList *self = (AhoghillQueueList *) object;

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

    priv->grid = nbtk_grid_new ();
    clutter_container_add_actor (CLUTTER_CONTAINER (self),
                                 CLUTTER_ACTOR (priv->grid));

    priv->items = g_ptr_array_new ();
}


void
ahoghill_queue_list_add_item (AhoghillQueueList *list,
                              BklItem           *item)
{
    AhoghillQueueListPrivate *priv = list->priv;
    AhoghillQueueTile *tile;

    tile = g_object_new (AHOGHILL_TYPE_QUEUE_TILE, NULL);
    /* ahoghill_queue_tile_set_item (tile, item); */

    g_ptr_array_add (priv->items, tile);

    clutter_container_add_actor (CLUTTER_CONTAINER (priv->grid),
                                 CLUTTER_ACTOR (tile));
    clutter_actor_show ((ClutterActor *) tile);
}
