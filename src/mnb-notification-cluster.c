/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Matthew Allum <mallum@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "mnb-notification-cluster.h"
#include "mnb-notification.h"
#include "moblin-netbook-notify-store.h"

G_DEFINE_TYPE (MnbNotificationCluster,   \
               mnb_notification_cluster, \
               NBTK_TYPE_WIDGET)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
   MNB_TYPE_NOTIFICATION_CLUSTER,    \
   MnbNotificationClusterPrivate))

#define CLUSTER_WIDTH 300
#define FADE_DURATION 300

struct _MnbNotificationClusterPrivate {
  ClutterGroup *notifiers;
  NbtkWidget   *control;
  NbtkWidget   *control_text;
  ClutterActor *lowlight;
  gint          n_notifiers;
  NbtkWidget   *active_notifier;

  ClutterActor *pending_removed;  /* notification pending removal on anim */
  gboolean      anim_lock;
};

typedef struct {
  MnbNotificationCluster *cluster;
  MoblinNetbookNotifyStore *store;
  guint id;
  guint reason;

} PostPonedData;

enum
{
  SYNC_INPUT_REGION,
  LAST_SIGNAL
};

static guint cluster_signals[LAST_SIGNAL] = { 0 };

static void
on_notification_closed (MoblinNetbookNotifyStore *store,
                        guint id,
                        guint reason,
                        MnbNotificationCluster *cluster);

static void
mnb_notification_cluster_get_property (GObject *object, guint property_id,
                                       GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_notification_cluster_set_property (GObject *object, guint property_id,
                                       const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_notification_cluster_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnb_notification_cluster_parent_class)->dispose (object);
}

static void
mnb_notification_cluster_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_notification_cluster_parent_class)->finalize (object);
}

static void
mnb_notification_cluster_paint (ClutterActor *actor)
{
  MnbNotificationClusterPrivate *priv = GET_PRIVATE (actor);

  if (CLUTTER_ACTOR_IS_VISIBLE (priv->control))
    clutter_actor_paint (CLUTTER_ACTOR(priv->control));

  if (priv->notifiers && CLUTTER_ACTOR_IS_VISIBLE (priv->notifiers))
    clutter_actor_paint (CLUTTER_ACTOR(priv->notifiers));
}

static void
mnb_notification_cluster_get_preferred_width (ClutterActor *actor,
                                              ClutterUnit   for_height,
                                              ClutterUnit  *min_width,
                                              ClutterUnit  *natural_width)
{
  *min_width = CLUTTER_UNITS_FROM_DEVICE(CLUSTER_WIDTH);
  *natural_width = CLUTTER_UNITS_FROM_DEVICE(CLUSTER_WIDTH);
}

static void
mnb_notification_cluster_get_preferred_height (ClutterActor *actor,
                                               ClutterUnit   for_width,
                                               ClutterUnit  *min_height,
                                               ClutterUnit  *natural_height)
{
  MnbNotificationClusterPrivate *priv = GET_PRIVATE (actor);

  *min_height = 0;
  *natural_height = 0;

  if (priv->notifiers)
    {
      ClutterUnit m_height, p_height;

      clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->notifiers),
                                          CLUSTER_WIDTH, &m_height, &p_height);

      *min_height += m_height;
      *natural_height += p_height;
    }

  if (priv->control && CLUTTER_ACTOR_IS_VISIBLE (priv->control))
    {
      *min_height
           = clutter_actor_get_yu (CLUTTER_ACTOR (priv->control))
             + clutter_actor_get_heightu (CLUTTER_ACTOR (priv->control));
      *natural_height
           = clutter_actor_get_yu (CLUTTER_ACTOR (priv->control))
             + clutter_actor_get_heightu (CLUTTER_ACTOR (priv->control));
    }
}


static void
mnb_notification_cluster_allocate (ClutterActor          *actor,
                                   const ClutterActorBox *box,
                                   gboolean               origin_changed)
{
  MnbNotificationClusterPrivate *priv = GET_PRIVATE (actor);
  ClutterActorClass *klass;

  klass = CLUTTER_ACTOR_CLASS (mnb_notification_cluster_parent_class);

  klass->allocate (actor, box, origin_changed);

  /* <rant>*sigh* and composite actors used to be so simple...</rant> */

  if (priv->control)
    {
      ClutterActorBox control_box = {
        clutter_actor_get_x (CLUTTER_ACTOR(priv->control)),
        clutter_actor_get_y (CLUTTER_ACTOR(priv->control)),
        clutter_actor_get_x (CLUTTER_ACTOR(priv->control)) +
          clutter_actor_get_width (CLUTTER_ACTOR(priv->control)),
        clutter_actor_get_y (CLUTTER_ACTOR(priv->control)) +
          clutter_actor_get_height (CLUTTER_ACTOR(priv->control))
      };

      clutter_actor_allocate (CLUTTER_ACTOR(priv->control),
                              &control_box, origin_changed);
    }

  if (priv->notifiers)
    {
      ClutterUnit m_height, p_height;
      ClutterActorBox notifier_box = { 0, };

      clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->notifiers),
                                          CLUSTER_WIDTH, &m_height, &p_height);

      notifier_box.x2 = CLUTTER_UNITS_FROM_DEVICE (CLUSTER_WIDTH);
      notifier_box.y2 = p_height;

      clutter_actor_allocate (CLUTTER_ACTOR(priv->notifiers),
                              &notifier_box, origin_changed);
    }
}

static void
mnb_notification_cluster_pick (ClutterActor       *actor,
                               const ClutterColor *color)
{
  CLUTTER_ACTOR_CLASS (mnb_notification_cluster_parent_class)->pick (actor,
                                                                     color);

  mnb_notification_cluster_paint (actor);
}


static void
mnb_notification_cluster_class_init (MnbNotificationClusterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *clutter_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbNotificationClusterPrivate));

  object_class->get_property = mnb_notification_cluster_get_property;
  object_class->set_property = mnb_notification_cluster_set_property;
  object_class->dispose = mnb_notification_cluster_dispose;
  object_class->finalize = mnb_notification_cluster_finalize;

  clutter_class->allocate = mnb_notification_cluster_allocate;
  clutter_class->paint = mnb_notification_cluster_paint;
  clutter_class->get_preferred_height
    = mnb_notification_cluster_get_preferred_height;
  clutter_class->get_preferred_width
    = mnb_notification_cluster_get_preferred_width;
  clutter_class->pick = mnb_notification_cluster_pick;

  cluster_signals[SYNC_INPUT_REGION] =
    g_signal_new ("sync-input-region",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbNotificationClusterClass,
                                   sync_input_region),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

}

static gint
id_compare (gconstpointer a, gconstpointer b)
{
  MnbNotification *notification = MNB_NOTIFICATION (a);
  guint find_id = GPOINTER_TO_INT (b);
  return mnb_notification_get_id (notification) - find_id;
}

static NbtkWidget*
find_widget (ClutterGroup *container, guint32 id)
{
  GList *children, *l;
  NbtkWidget *w;

  children = clutter_container_get_children (CLUTTER_CONTAINER(container));
  l = g_list_find_custom (children, GINT_TO_POINTER (id), id_compare);
  w = l ? l->data : NULL;
  g_list_free (children);
  return w;
}

static void
on_closed (MnbNotification *notification, MoblinNetbookNotifyStore *store)
{
  moblin_netbook_notify_store_close (store,
                                     mnb_notification_get_id (notification),
                                     ClosedDismissed);
}

static void
on_action (MnbNotification *notification,
           gchar           *action,
           MoblinNetbookNotifyStore *store)
{
  moblin_netbook_notify_store_action (store,
                                      mnb_notification_get_id (notification),
                                      action);
}


static void
on_control_appear_anim_completed (ClutterAnimation *anim,
                                  MnbNotificationCluster *cluster)
{
  g_signal_emit (cluster, cluster_signals[SYNC_INPUT_REGION], 0);
}

static void
on_notification_added (MoblinNetbookNotifyStore *store,
                       Notification             *notification,
                       MnbNotificationCluster   *cluster)
{
  MnbNotificationClusterPrivate *priv = GET_PRIVATE (cluster);
  NbtkWidget *w;
  ClutterAnimation *anim;

  /* Handled by mnb-notification-urgent */
  if (notification->is_urgent)
    return;

  w = find_widget (priv->notifiers, notification->id);

  if (!w)
    {
      w = mnb_notification_new ();
      g_signal_connect (w, "closed", G_CALLBACK (on_closed), store);
      g_signal_connect (w, "action", G_CALLBACK (on_action), store);

      clutter_container_add_actor (CLUTTER_CONTAINER (priv->notifiers),
                                   CLUTTER_ACTOR(w));
      clutter_actor_hide (CLUTTER_ACTOR(w));

      clutter_actor_set_width (CLUTTER_ACTOR(w), CLUSTER_WIDTH);

      mnb_notification_update (MNB_NOTIFICATION (w), notification);

      priv->n_notifiers++;
    }
  else
    {
      mnb_notification_update (MNB_NOTIFICATION (w), notification);
      return;
    }

  if (priv->n_notifiers == 1)
    {
      /* show just the single notification */
      priv->active_notifier = w;
      clutter_actor_set_opacity (CLUTTER_ACTOR(w), 0);
      clutter_actor_show (CLUTTER_ACTOR(w));

      clutter_actor_animate (CLUTTER_ACTOR(w),
                             CLUTTER_EASE_IN_SINE,
                             FADE_DURATION,
                             "opacity", 0xff,
                             NULL);

      g_signal_emit (cluster, cluster_signals[SYNC_INPUT_REGION], 0);
     }
  else if (priv->n_notifiers == 2)
    {
      /* slide the control into view */
      nbtk_label_set_text (NBTK_LABEL(priv->control_text),
                           "1 pending message");

      clutter_actor_set_opacity (CLUTTER_ACTOR(priv->control), 0);
      clutter_actor_set_y (CLUTTER_ACTOR(priv->control),
              clutter_actor_get_height (CLUTTER_ACTOR(priv->active_notifier))
                 - clutter_actor_get_height (CLUTTER_ACTOR(priv->control)));

      clutter_actor_show (CLUTTER_ACTOR(priv->control));

      anim = clutter_actor_animate (CLUTTER_ACTOR(priv->control),
                                    CLUTTER_EASE_IN_SINE,
                                    FADE_DURATION,
                                    "opacity", 0xff,
                                    "y", clutter_actor_get_height
                                    (CLUTTER_ACTOR(priv->active_notifier))- 30,
                                    NULL);
      g_signal_connect (anim,
                        "completed",
                        G_CALLBACK (on_control_appear_anim_completed),
                        cluster);
    }
  else
    {
      /* simply update the control */
      gchar *msg;

      msg = g_strdup_printf ("%i pending messages", priv->n_notifiers-1);

      nbtk_label_set_text (NBTK_LABEL(priv->control_text), msg);

      g_free (msg);
    }

}

static gboolean
postponed_timeout (PostPonedData *data)
{
  MnbNotificationClusterPrivate *priv = GET_PRIVATE (data->cluster);

  /* Used to postpone removal animations as to avoid races */

  if (priv->anim_lock == TRUE)
    return TRUE;

  on_notification_closed (data->store,
                          data->id,
                          data->reason,
                          data->cluster);

  g_slice_free (PostPonedData, data);

  return FALSE;
}

static void
on_control_disappear_anim_completed (ClutterAnimation *anim,
                                     MnbNotificationCluster *cluster)
{
  MnbNotificationClusterPrivate *priv = GET_PRIVATE (cluster);

  /* Animation has completed so now we really remove notification
   * from group.
   */
  if (priv->pending_removed)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->notifiers),
                                      priv->pending_removed);
      priv->pending_removed = NULL;
    }

  /* Update flag for any pending animations */
  priv->anim_lock = FALSE;

  /* We may have changed size to resync the input area */
  g_signal_emit (cluster, cluster_signals[SYNC_INPUT_REGION], 0);
}

static void
on_notification_closed (MoblinNetbookNotifyStore *store,
                        guint id,
                        guint reason,
                        MnbNotificationCluster *cluster)
{
  MnbNotificationClusterPrivate *priv = GET_PRIVATE (cluster);
  ClutterAnimation *anim;
  NbtkWidget *w;

  if (priv->anim_lock == TRUE)
    {
      /* We are already running a removal animation - therefor we postpone
       * for the length of animation as to avoid races.
       */
      PostPonedData *d;

      d = g_slice_new (PostPonedData);
      d->id      = id;
      d->reason  = reason;
      d->cluster = cluster;
      d->store   = store;

      g_timeout_add (FADE_DURATION+50, (GSourceFunc)postponed_timeout, d);
      return;
    }

  w = find_widget (priv->notifiers, id);

  if (w)
    {
      /* needed for callback */
      priv->pending_removed = CLUTTER_ACTOR(w);

      priv->n_notifiers--;

      /* fade out closed notifier */
      if (w == priv->active_notifier)
        {
          anim = clutter_actor_animate (CLUTTER_ACTOR(w),
                                        CLUTTER_EASE_IN_SINE,
                                        FADE_DURATION,
                                        "opacity", 0,
                                        NULL);

          priv->anim_lock = TRUE;

          g_signal_connect (anim,
                            "completed",
                            G_CALLBACK
                            (on_control_disappear_anim_completed),
                            cluster);
        }
      else
        {
          /* simply remove no need for anim */
          clutter_container_remove_actor (CLUTTER_CONTAINER (priv->notifiers),
                                          CLUTTER_ACTOR(w));
        }

      /* Fade in newer notifier from below stack */
      if (w == priv->active_notifier && priv->n_notifiers > 0)
        {
          gint prev_height, new_height;

          prev_height = clutter_actor_get_height (CLUTTER_ACTOR(w));

          priv->active_notifier = NBTK_WIDGET (
            clutter_group_get_nth_child (CLUTTER_GROUP (priv->notifiers),
                                         1)); /* Next, not 0 */

          if (priv->active_notifier)
            {
              clutter_actor_set_opacity (CLUTTER_ACTOR(priv->active_notifier),
                                         0);
              clutter_actor_show (CLUTTER_ACTOR(priv->active_notifier));
              clutter_actor_animate (CLUTTER_ACTOR(priv->active_notifier),
                                     CLUTTER_EASE_IN_SINE,
                                     FADE_DURATION,
                                     "opacity", 0xff,
                                     NULL);

              new_height = clutter_actor_get_height
                                    (CLUTTER_ACTOR(priv->active_notifier));

              if (prev_height != new_height && priv->n_notifiers > 1)
                {
                  gint new_y;

                  new_y = clutter_actor_get_y (CLUTTER_ACTOR(priv->control))
                                 - (prev_height - new_height);

                  /* if (new_y < 0) new_y = 0; */

                  clutter_actor_animate (CLUTTER_ACTOR(priv->control),
                                         CLUTTER_EASE_IN_SINE,
                                         FADE_DURATION,
                                         "y", new_y,
                                         NULL);
                }
            }
        }

      if (priv->n_notifiers == 0)
        {
          /* Animation above would have removed */
          priv->active_notifier = NULL;
        }
      else if (priv->n_notifiers == 1)
        {
          /* slide the control out of view */
          clutter_actor_animate (CLUTTER_ACTOR(priv->control),
                                 CLUTTER_EASE_IN_SINE,
                                 FADE_DURATION,
                                 "opacity", 0x0,
                                 "y", clutter_actor_get_height
                                        (CLUTTER_ACTOR(priv->active_notifier))
                                      - clutter_actor_get_height
                                        (CLUTTER_ACTOR(priv->control)),
                                 NULL);

          /* clutter_actor_hide (CLUTTER_ACTOR(priv->control)); */
          /* make need above and input regiion sync from above */
        }
      else
        {
          /* Just Update control text */
          gchar *msg;
          msg = g_strdup_printf ("%i pending messages", priv->n_notifiers);
          nbtk_label_set_text (NBTK_LABEL(priv->control_text), msg);
          g_free (msg);
        }
    }
}

static void
on_dismiss_all_foreach (ClutterActor *notifier)
{
  g_signal_emit_by_name (notifier, "closed", 0);
}

static void
on_dismiss_all_click (ClutterActor *button, MnbNotificationCluster *cluster)
{
  MnbNotificationClusterPrivate *priv = GET_PRIVATE (cluster);

  /* FIXME: Should actually run an animation here */
  clutter_actor_hide (CLUTTER_ACTOR(cluster));

  clutter_container_foreach (CLUTTER_CONTAINER(priv->notifiers),
                             (ClutterCallback)on_dismiss_all_foreach,
                             NULL);
}

void
mnb_notification_cluster_set_store (MnbNotificationCluster    *self,
                                    MoblinNetbookNotifyStore  *notify_store)
{
  g_signal_connect (notify_store, 
                    "notification-added", 
                    G_CALLBACK (on_notification_added), 
                    self);

  g_signal_connect (notify_store, 
                    "notification-closed", 
                    G_CALLBACK (on_notification_closed), 
                    self);
}


static void
mnb_notification_cluster_init (MnbNotificationCluster *self)
{
  NbtkWidget *widget;
  MnbNotificationClusterPrivate *priv = GET_PRIVATE (self);

  priv->notifiers = CLUTTER_GROUP(clutter_group_new ());

  clutter_actor_set_parent (CLUTTER_ACTOR(priv->notifiers),
                            CLUTTER_ACTOR(self));

  /* 'Overflow' control */
  priv->control = nbtk_table_new ();

  nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->control),
                                    "notification-control");

  widget = nbtk_button_new ();
  nbtk_button_set_label (NBTK_BUTTON (widget), "Dismiss All");
  nbtk_table_add_widget (NBTK_TABLE (priv->control), widget, 0, 1);

  g_signal_connect (widget, "clicked",
                    G_CALLBACK (on_dismiss_all_click), self);

  priv->control_text = nbtk_label_new ("");
  nbtk_table_add_widget (NBTK_TABLE (priv->control), priv->control_text, 0, 0);

  clutter_actor_set_width (CLUTTER_ACTOR(priv->control), CLUSTER_WIDTH);

  clutter_actor_set_parent (CLUTTER_ACTOR(priv->control),
                            CLUTTER_ACTOR(self));

  clutter_actor_hide (CLUTTER_ACTOR(priv->control));

  clutter_actor_set_reactive (CLUTTER_ACTOR(priv->notifiers), TRUE);
  clutter_actor_set_reactive (CLUTTER_ACTOR(self), TRUE);
}

ClutterActor*
mnb_notification_cluster_new (void)
{
  return g_object_new (MNB_TYPE_NOTIFICATION_CLUSTER, NULL);
}

