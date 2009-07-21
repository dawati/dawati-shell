#include <bickley/bkl-item.h>

#include "ahoghill-playlist-np.h"
#include "ahoghill-queue-list.h"
#include "ahoghill-queue-tile.h"

enum {
    PROP_0,
};

struct _AhoghillQueueListPrivate {
    AhoghillPlaylistNp *now_playing;
    NbtkWidget *scroller;
    NbtkWidget *listview;

    int count;
    ClutterModel *model;
};

#define LIST_GAP 15.0

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AHOGHILL_TYPE_QUEUE_LIST, AhoghillQueueListPrivate))
G_DEFINE_TYPE (AhoghillQueueList, ahoghill_queue_list, NBTK_TYPE_WIDGET);

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
ahoghill_queue_list_allocate (ClutterActor           *actor,
                              const ClutterActorBox  *box,
                              ClutterAllocationFlags  flags)
{
    AhoghillQueueList *list = (AhoghillQueueList *) actor;
    AhoghillQueueListPrivate *priv = list->priv;
    NbtkPadding padding = {0, };
    float height, np_height, width;

    nbtk_widget_get_padding ((NbtkWidget *) actor, &padding);

    CLUTTER_ACTOR_CLASS (ahoghill_queue_list_parent_class)->allocate
        (actor, box, flags);

    width = clutter_actor_get_width (actor) - (padding.left + padding.right);
    clutter_actor_allocate_preferred_size ((ClutterActor *) priv->now_playing,
                                           flags);
    clutter_actor_set_width ((ClutterActor *) priv->now_playing, width);
    clutter_actor_set_position ((ClutterActor *) priv->now_playing,
                                padding.left, padding.top);

    height = clutter_actor_get_height (actor);

    if (CLUTTER_ACTOR_IS_MAPPED ((ClutterActor *) priv->now_playing)) {
        np_height = clutter_actor_get_height ((ClutterActor *) priv->now_playing);
    } else {
        np_height = 0.0;
    }

    clutter_actor_allocate_preferred_size ((ClutterActor *) priv->scroller,
                                           flags);
    clutter_actor_set_width ((ClutterActor *) priv->scroller, width);
    clutter_actor_set_height ((ClutterActor *) priv->scroller,
                              height - (padding.top + padding.bottom +
                                        np_height));

    clutter_actor_set_position ((ClutterActor *) priv->scroller,
                                padding.left, np_height + padding.top);
}

static void
ahoghill_queue_list_paint (ClutterActor *actor)
{
    AhoghillQueueList *list = (AhoghillQueueList *) actor;
    AhoghillQueueListPrivate *priv = list->priv;

    CLUTTER_ACTOR_CLASS (ahoghill_queue_list_parent_class)->paint (actor);

    if (CLUTTER_ACTOR_IS_MAPPED ((ClutterActor *) priv->now_playing)) {
        clutter_actor_paint ((ClutterActor *) priv->now_playing);
    }

    clutter_actor_paint ((ClutterActor *) priv->scroller);
}

static void
ahoghill_queue_list_pick (ClutterActor       *actor,
                          const ClutterColor *color)
{
    AhoghillQueueList *list = (AhoghillQueueList *) actor;
    AhoghillQueueListPrivate *priv = list->priv;

    CLUTTER_ACTOR_CLASS (ahoghill_queue_list_parent_class)->paint (actor);

    if (CLUTTER_ACTOR_IS_MAPPED ((ClutterActor *) priv->now_playing)) {
        clutter_actor_paint ((ClutterActor *) priv->now_playing);
    }

    clutter_actor_paint ((ClutterActor *) priv->scroller);
}

static void
ahoghill_queue_list_map (ClutterActor *actor)
{
    AhoghillQueueList *list = (AhoghillQueueList *) actor;
    AhoghillQueueListPrivate *priv = list->priv;

    CLUTTER_ACTOR_CLASS (ahoghill_queue_list_parent_class)->map (actor);

    clutter_actor_map ((ClutterActor *) priv->now_playing);
    clutter_actor_map ((ClutterActor *) priv->scroller);
}

static void
ahoghill_queue_list_unmap (ClutterActor *actor)
{
    AhoghillQueueList *list = (AhoghillQueueList *) actor;
    AhoghillQueueListPrivate *priv = list->priv;

    CLUTTER_ACTOR_CLASS (ahoghill_queue_list_parent_class)->unmap (actor);

    clutter_actor_unmap ((ClutterActor *) priv->now_playing);
    clutter_actor_unmap ((ClutterActor *) priv->scroller);
}

static void
ahoghill_queue_list_class_init (AhoghillQueueListClass *klass)
{
    GObjectClass *o_class = (GObjectClass *) klass;
    ClutterActorClass *a_class = (ClutterActorClass *) klass;

    o_class->dispose = ahoghill_queue_list_dispose;
    o_class->finalize = ahoghill_queue_list_finalize;
    o_class->set_property = ahoghill_queue_list_set_property;
    o_class->get_property = ahoghill_queue_list_get_property;

    a_class->allocate = ahoghill_queue_list_allocate;
    a_class->paint = ahoghill_queue_list_paint;
    a_class->pick = ahoghill_queue_list_pick;
    a_class->map = ahoghill_queue_list_map;
    a_class->unmap = ahoghill_queue_list_unmap;

    g_type_class_add_private (klass, sizeof (AhoghillQueueListPrivate));
}

static void
ahoghill_queue_list_init (AhoghillQueueList *self)
{
    AhoghillQueueListPrivate *priv;
    NbtkPadding padding = { 0, };

    self->priv = GET_PRIVATE (self);
    priv = self->priv;

    priv->count = 0;
    nbtk_widget_get_padding ((NbtkWidget *) self, &padding);

    priv->now_playing = g_object_new (AHOGHILL_TYPE_PLAYLIST_NP, NULL);
    clutter_actor_set_parent ((ClutterActor *) priv->now_playing,
                              (ClutterActor *) self);
    clutter_actor_set_position ((ClutterActor *) priv->now_playing,
                                padding.left, padding.top);
    clutter_actor_hide ((ClutterActor *) priv->now_playing);

    priv->model = clutter_list_model_new (1, BKL_TYPE_ITEM, "item");

    priv->scroller = nbtk_scroll_view_new ();
    clutter_actor_set_parent ((ClutterActor *) priv->scroller,
                              (ClutterActor *) self);
    clutter_actor_set_position ((ClutterActor *) priv->scroller,
                                padding.left, 0);

    priv->listview = nbtk_list_view_new ();
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->scroller),
                                 CLUTTER_ACTOR (priv->listview));

    nbtk_list_view_set_model (NBTK_LIST_VIEW (priv->listview), priv->model);
    nbtk_list_view_set_item_type (NBTK_LIST_VIEW (priv->listview),
                                  AHOGHILL_TYPE_QUEUE_TILE);
    nbtk_list_view_add_attribute (NBTK_LIST_VIEW (priv->listview),
                                  "item", 0);
}

void
ahoghill_queue_list_add_item (AhoghillQueueList *list,
                              BklItem           *item,
                              int                index)
{
    AhoghillQueueListPrivate *priv = list->priv;

    clutter_model_insert (priv->model, index, 0, item, -1);
    priv->count++;
}

void
ahoghill_queue_list_remove (AhoghillQueueList *list,
                            int                index)
{
    AhoghillQueueListPrivate *priv = list->priv;

    priv->count--;
    clutter_model_remove (priv->model, index);
}

int
ahoghill_queue_list_get_item_count (AhoghillQueueList *list)
{
    AhoghillQueueListPrivate *priv = list->priv;

    return priv->count;
}

AhoghillPlaylistNp *
ahoghill_queue_list_get_np (AhoghillQueueList *list)
{
    AhoghillQueueListPrivate *priv = list->priv;

    return priv->now_playing;
}

void
ahoghill_queue_list_now_playing_set_showing (AhoghillQueueList *list,
                                             gboolean           showing)
{
    AhoghillQueueListPrivate *priv = list->priv;

    if (showing) {
        clutter_actor_show ((ClutterActor *) priv->now_playing);
    } else {
        clutter_actor_hide ((ClutterActor *) priv->now_playing);
    }
}
