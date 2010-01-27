/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Rob bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "mps-view-bridge.h"
#include "mps-tweet-card.h"

G_DEFINE_TYPE (MpsViewBridge, mps_view_bridge, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPS_TYPE_VIEW_BRIDGE, MpsViewBridgePrivate))

typedef struct _MpsViewBridgePrivate MpsViewBridgePrivate;

struct _MpsViewBridgePrivate {
  MojitoClientView *view;
  ClutterContainer *container;

  GHashTable *item_uid_to_actor;
  ClutterScore *score;

  GList *actors_to_animate;
  ClutterTimeline *current_timeline;
};

enum
{
  PROP_0,
  PROP_VIEW,
  PROP_CONTAINER
};

#define THRESHOLD 10
#define CARD_HEIGHT 100.0

static void
mps_view_bridge_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  MpsViewBridge *bridge = (MpsViewBridge *)object;

  switch (property_id) {
    case PROP_VIEW:
      g_value_set_object (value, mps_view_bridge_get_view (bridge));
      break;
    case PROP_CONTAINER:
      g_value_set_object (value, mps_view_bridge_get_container (bridge));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mps_view_bridge_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  MpsViewBridge *bridge = (MpsViewBridge *)object;

  switch (property_id) {
    case PROP_VIEW:
      mps_view_bridge_set_view (bridge,
                                (MojitoClientView *)g_value_get_object (value));
      break;
    case PROP_CONTAINER:
      mps_view_bridge_set_container (bridge,
                                     (ClutterContainer *)g_value_get_object (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mps_view_bridge_dispose (GObject *object)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (object);

  if (priv->score)
  {
    g_object_unref (priv->score);
    priv->score = NULL;
  }

  if (priv->item_uid_to_actor)
  {
    g_hash_table_unref (priv->item_uid_to_actor);
    priv->item_uid_to_actor = NULL;
  }

  G_OBJECT_CLASS (mps_view_bridge_parent_class)->dispose (object);
}

static void
mps_view_bridge_finalize (GObject *object)
{
  G_OBJECT_CLASS (mps_view_bridge_parent_class)->finalize (object);
}

static void
mps_view_bridge_class_init (MpsViewBridgeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpsViewBridgePrivate));

  object_class->get_property = mps_view_bridge_get_property;
  object_class->set_property = mps_view_bridge_set_property;
  object_class->dispose = mps_view_bridge_dispose;
  object_class->finalize = mps_view_bridge_finalize;
}

static void
mps_view_bridge_init (MpsViewBridge *self)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (self);

  priv->item_uid_to_actor = g_hash_table_new_full (g_str_hash,
                                                   g_str_equal,
                                                   g_free,
                                                   (GDestroyNotify)clutter_actor_destroy);
}

MpsViewBridge *
mps_view_bridge_new (void)
{
  return g_object_new (MPS_TYPE_VIEW_BRIDGE, NULL);
}

static gint
_mojito_item_sort_compare_func (MojitoItem *a,
                                MojitoItem *b)
{
  if (a->date.tv_sec < b->date.tv_sec)
  {
    return -1;
  } else if (a->date.tv_sec == b->date.tv_sec) {
    return 0;
  } else {
    return 1;
  }
}

#if 0
static void
_view_items_added_cb (MojitoClientView *view,
                      GList            *items,
                      MpsViewBridge    *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);
  GList *l = NULL;
  gint i = 0;
  ClutterTimeline *timeline;
  ClutterTimeline *last_timeline = NULL;
  gint item_count = 0;


  /* TODO: Add some kind of delaying here */
  items = g_list_sort (items,
                       (GCompareFunc)_mojito_item_sort_compare_func);

  if (!priv->score)
  {
    priv->score = clutter_score_new ();
  } else {
    GSList *timelines;

    timelines = clutter_score_list_timelines (priv->score);
    last_timeline = (ClutterTimeline *)(g_slist_last (timelines))->data;
    g_slist_free (timelines);
  }

  item_count = g_list_length (items);

  if (item_count > THRESHOLD)
  {
    for (i = 0, l = items;
         i < item_count - THRESHOLD;
         i++, l=l->next)
    {
      ClutterActor *actor;
      MojitoItem   *item = (MojitoItem *)l->data;
      actor = g_object_new (MPS_TYPE_TWEET_CARD,
                            "item", item,
                            NULL);
      clutter_container_add_actor (CLUTTER_CONTAINER (priv->container),
                                   actor);
      clutter_container_lower_child (priv->container, actor, NULL);
      clutter_container_child_set (CLUTTER_CONTAINER (priv->container),
                                   actor,
                                   "x-fill", TRUE,
                                   "y-fill", FALSE,
                                   "expand", FALSE,
                                   NULL);
      clutter_actor_set_height (actor, 100);
    }
  } else {
    l = items;
  }

  for (; l != NULL; l = l->next)
  {
    ClutterActor *actor;
    MojitoItem   *item = (MojitoItem *)l->data;
    ClutterTimeline *opacity_timeline;
    ClutterAnimation *animation;
    ClutterBehaviour *behave;
    ClutterAlpha *alpha;

    actor = g_object_new (MPS_TYPE_TWEET_CARD,
                          "item", item,
                          NULL);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->container),
                                 actor);
    clutter_container_lower_child (priv->container, actor, NULL);
    clutter_container_child_set (CLUTTER_CONTAINER (priv->container),
                                 actor,
                                 "x-fill", TRUE,
                                 "y-fill", FALSE,
                                 "expand", FALSE,
                                 NULL);
    clutter_actor_set_height (actor, 0);
    clutter_actor_set_opacity (actor, 0);

    animation = clutter_actor_animate (actor,
                                       CLUTTER_LINEAR,
                                       600,
                                       "height", 100.0,
                                       NULL);
    timeline = clutter_animation_get_timeline (animation);

    clutter_timeline_set_delay (timeline, 600);
    clutter_timeline_stop (timeline);
    clutter_timeline_add_marker_at_time (timeline, "opacity-trigger", 500);

    clutter_score_append (priv->score,
                          last_timeline,
                          timeline);

    opacity_timeline = clutter_timeline_new (100);
    clutter_timeline_stop (opacity_timeline);

    alpha = clutter_alpha_new_full (opacity_timeline,
                                    CLUTTER_LINEAR);
    behave = clutter_behaviour_opacity_new (alpha, 
                                            0,
                                            255);
    clutter_behaviour_apply (behave, actor);
    clutter_score_append_at_marker (priv->score,
                                    timeline,
                                    "opacity-trigger",
                                    opacity_timeline);
    last_timeline = timeline;
  }

  clutter_score_start (priv->score);
}
#endif



static void _do_next_card_animation (MpsViewBridge *bridge);

static void
_animation_completed_cb (ClutterAnimation *animation,
                         MpsViewBridge    *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  priv->current_timeline = NULL;

  _do_next_card_animation (bridge);
}

static void
_do_next_card_animation (MpsViewBridge *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);
  ClutterActor *actor;
  ClutterAnimation *animation;
  ClutterTimeline *timeline;
  ClutterTimeline *opacity_timeline;
  ClutterAlpha *alpha;
  ClutterBehaviour *behave;

  if (!priv->actors_to_animate)
  {
    return;
  }

  /* Get current actor and update head of list */
  actor = (ClutterActor *)priv->actors_to_animate->data;
  priv->actors_to_animate = g_list_remove (priv->actors_to_animate,
                                           actor);

  animation = clutter_actor_animate (actor,
                                     CLUTTER_LINEAR,
                                     400,
                                     "height", CARD_HEIGHT,
                                     NULL);
  timeline = clutter_animation_get_timeline (animation);
  clutter_timeline_stop (timeline);
  clutter_timeline_set_delay (timeline, 600);
  clutter_timeline_start (timeline);
  priv->current_timeline = timeline;

  opacity_timeline = clutter_timeline_new (150);
  clutter_timeline_set_delay (opacity_timeline, 850);
  alpha = clutter_alpha_new_full (opacity_timeline, CLUTTER_EASE_OUT_QUAD);
  g_object_unref (opacity_timeline);
  behave = clutter_behaviour_opacity_new (alpha,
                                          0,
                                          255);
  clutter_behaviour_apply (behave, actor);
  g_signal_connect_swapped (opacity_timeline,
                            "completed",
                            (GCallback)g_object_unref,
                            behave);
  clutter_timeline_start (opacity_timeline);

  g_signal_connect_after (animation,
                          "completed",
                          (GCallback)_animation_completed_cb,
                          bridge);
}

static void
_view_items_added_cb (MojitoClientView *view,
                      GList            *items,
                      MpsViewBridge    *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);
  gint item_count = 0;
  GList *l;
  gint i = 0;

  g_debug (G_STRLOC ": %s called", G_STRFUNC);

  /* Oldest first */
  items = g_list_sort (items,
                       (GCompareFunc)_mojito_item_sort_compare_func);

  item_count = g_list_length (items);


  for (l = items; l; l = l->next)
  {
    MojitoItem *item = (MojitoItem *)l->data;
    ClutterActor *actor;

    actor = g_object_new (MPS_TYPE_TWEET_CARD,
                          "item", item,
                          NULL);
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->container),
                                 actor);

    g_hash_table_insert (priv->item_uid_to_actor,
                         g_strdup (item->uuid),
                         actor);

    /* Position it at the top */
    clutter_container_lower_child (priv->container, actor, NULL);

    clutter_container_child_set (CLUTTER_CONTAINER (priv->container),
                                 actor,
                                 "x-fill", TRUE,
                                 "y-fill", FALSE,
                                 "expand", FALSE,
                                 NULL);

    if (i < item_count - THRESHOLD)
    {
      clutter_actor_set_height (actor, CARD_HEIGHT);
    } else {
      clutter_actor_set_height (actor, 0);
      priv->actors_to_animate = g_list_append (priv->actors_to_animate,
                                               actor);
      clutter_actor_set_opacity (actor, 0);
    }

    i++;
  }

  /* Deal with the overflow on the pending items */
  while (g_list_length (priv->actors_to_animate) > THRESHOLD)
  {
    ClutterActor *actor = (ClutterActor *)priv->actors_to_animate->data;

    clutter_actor_set_height (actor, CARD_HEIGHT);
    clutter_actor_set_opacity (actor, 255);

    priv->actors_to_animate = g_list_remove (priv->actors_to_animate, actor);
  }

  /* We have a started chain of animations */
  if (!priv->current_timeline)
    _do_next_card_animation (bridge);
}

static void
_view_items_removed_cb (MojitoClientView *view,
                        GList            *items,
                        MpsViewBridge    *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

}

static void
_view_items_changed_cb (MojitoClientView *view,
                        GList     *items,
                        MpsViewBridge *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);
  GList *l;

  for (l = items; l; l = l->next)
  {
    MojitoItem *item = (MojitoItem *)l->data;
    ClutterActor *actor;

    actor = g_hash_table_lookup (priv->item_uid_to_actor,
                                 item->uuid);

    if (actor)
    {
      g_object_set (actor,
                    "item", item,
                    NULL);
    }
  }
}


void
mps_view_bridge_set_view (MpsViewBridge    *bridge,
                          MojitoClientView *view)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  /* Can only be called once */
  g_assert (!priv->view);
  priv->view = g_object_ref (view);

  g_signal_connect (priv->view,
                    "items-added",
                    (GCallback)_view_items_added_cb,
                    bridge);
  g_signal_connect (priv->view,
                    "items-removed",
                    (GCallback)_view_items_removed_cb,
                    bridge);
  g_signal_connect (priv->view,
                    "items-changed",
                    (GCallback)_view_items_changed_cb,
                    bridge);

  mojito_client_view_start (priv->view);
}

void
mps_view_bridge_set_container (MpsViewBridge    *bridge,
                               ClutterContainer *container)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  /* Can only be called once */
  g_assert (!priv->container);
  priv->container = g_object_ref (container);
}

MojitoClientView *
mps_view_bridge_get_view (MpsViewBridge *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  return priv->view;
}

ClutterContainer *
mps_view_bridge_get_container (MpsViewBridge *bridge)
{
  MpsViewBridgePrivate *priv = GET_PRIVATE (bridge);

  return priv->container;
}


