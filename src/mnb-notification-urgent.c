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

#include "mnb-notification-urgent.h"
#include "mnb-notification.h"
#include "moblin-netbook-notify-store.h"

G_DEFINE_TYPE (MnbNotificationUrgent,   \
               mnb_notification_urgent, \
               NBTK_TYPE_WIDGET)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
   MNB_TYPE_NOTIFICATION_URGENT,    \
   MnbNotificationUrgentPrivate))

#define URGENT_WIDTH 400
#define FADE_DURATION 300

struct _MnbNotificationUrgentPrivate {
  ClutterGroup *notifiers;
  NbtkWidget   *active;
  gint          n_notifiers;
};

enum
{
  SYNC_INPUT_REGION,
  LAST_SIGNAL
};

static guint urgent_signals[LAST_SIGNAL] = { 0 };

static void
mnb_notification_urgent_get_property (GObject *object, guint property_id,
                                       GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_notification_urgent_set_property (GObject *object, guint property_id,
                                       const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_notification_urgent_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnb_notification_urgent_parent_class)->dispose (object);
}

static void
mnb_notification_urgent_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_notification_urgent_parent_class)->finalize (object);
}

static void
mnb_notification_urgent_paint (ClutterActor *actor)
{
  MnbNotificationUrgentPrivate *priv = GET_PRIVATE (actor);

  if (priv->notifiers && CLUTTER_ACTOR_IS_MAPPED (priv->notifiers))
      clutter_actor_paint (CLUTTER_ACTOR (priv->notifiers));
}

static void
mnb_notification_urgent_map (ClutterActor *actor)
{
  MnbNotificationUrgentPrivate *priv = GET_PRIVATE (actor);

  CLUTTER_ACTOR_CLASS (mnb_notification_urgent_parent_class)->map (actor);

  if (priv->notifiers)
      clutter_actor_map (CLUTTER_ACTOR (priv->notifiers));
}

static void
mnb_notification_urgent_unmap (ClutterActor *actor)
{
  MnbNotificationUrgentPrivate *priv = GET_PRIVATE (actor);

  CLUTTER_ACTOR_CLASS (mnb_notification_urgent_parent_class)->unmap (actor);

  if (priv->notifiers)
      clutter_actor_unmap (CLUTTER_ACTOR (priv->notifiers));
}

static void
mnb_notification_urgent_get_preferred_width (ClutterActor *actor,
                                             ClutterUnit   for_height,
                                             ClutterUnit  *min_width,
                                             ClutterUnit  *natural_width)
{
  *min_width = CLUTTER_UNITS_FROM_DEVICE(URGENT_WIDTH);
  *natural_width = CLUTTER_UNITS_FROM_DEVICE(URGENT_WIDTH);
}

static void
mnb_notification_urgent_get_preferred_height (ClutterActor *actor,
                                               ClutterUnit   for_width,
                                               ClutterUnit  *min_height,
                                               ClutterUnit  *natural_height)
{
  MnbNotificationUrgentPrivate *priv = GET_PRIVATE (actor);

  *min_height = 0;
  *natural_height = 0;

  if (priv->notifiers)
    {
      ClutterUnit m_height, p_height;

      clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->notifiers),
                                          URGENT_WIDTH, &m_height, &p_height);

      *min_height += m_height;
      *natural_height += p_height;
    }
}


static void
mnb_notification_urgent_allocate (ClutterActor          *actor,
                                  const ClutterActorBox *box,
                                  gboolean               origin_changed)
{
  MnbNotificationUrgentPrivate *priv = GET_PRIVATE (actor);
  ClutterActorClass *klass;

  klass = CLUTTER_ACTOR_CLASS (mnb_notification_urgent_parent_class);

  klass->allocate (actor, box, origin_changed);

  if (priv->notifiers)
    {
      ClutterUnit m_height, p_height;
      ClutterActorBox notifier_box = { 0, };

      clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->notifiers),
                                          URGENT_WIDTH, &m_height, &p_height);

      notifier_box.x2 = CLUTTER_UNITS_FROM_DEVICE (URGENT_WIDTH);
      notifier_box.y2 = p_height;

      clutter_actor_allocate (CLUTTER_ACTOR(priv->notifiers),
                              &notifier_box, origin_changed);
    }
}

static void
mnb_notification_urgent_pick (ClutterActor       *actor,
                               const ClutterColor *color)
{
  CLUTTER_ACTOR_CLASS (mnb_notification_urgent_parent_class)->pick (actor, 
                                                                     color);
  mnb_notification_urgent_paint (actor);
}


static void
mnb_notification_urgent_class_init (MnbNotificationUrgentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *clutter_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbNotificationUrgentPrivate));

  object_class->get_property = mnb_notification_urgent_get_property;
  object_class->set_property = mnb_notification_urgent_set_property;
  object_class->dispose = mnb_notification_urgent_dispose;
  object_class->finalize = mnb_notification_urgent_finalize;

  clutter_class->allocate = mnb_notification_urgent_allocate;
  clutter_class->paint = mnb_notification_urgent_paint;
  clutter_class->pick = mnb_notification_urgent_pick;
  clutter_class->get_preferred_height
    = mnb_notification_urgent_get_preferred_height;
  clutter_class->get_preferred_width
    = mnb_notification_urgent_get_preferred_width;
  clutter_class->map = mnb_notification_urgent_map;
  clutter_class->unmap = mnb_notification_urgent_unmap;

  urgent_signals[SYNC_INPUT_REGION] =
    g_signal_new ("sync-input-region",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbNotificationUrgentClass, 
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

#if 0
/*
 * This should either be connected or deleted.
 */
static void
on_control_appear_anim_completed (ClutterAnimation *anim,
                                  MnbNotificationUrgent *urgent)
{
  g_signal_emit (urgent, urgent_signals[SYNC_INPUT_REGION], 0);
}
#endif

static void
on_notification_added (MoblinNetbookNotifyStore *store, 
                       Notification             *notification, 
                       MnbNotificationUrgent   *urgent)
{
  MnbNotificationUrgentPrivate *priv = GET_PRIVATE (urgent);
  NbtkWidget *w;

  if (!notification->is_urgent)
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

      clutter_actor_set_width (CLUTTER_ACTOR(w), URGENT_WIDTH);

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
      priv->active = w;
      /* run appear anim ? */
      clutter_actor_show (CLUTTER_ACTOR(priv->notifiers));
      clutter_actor_show (CLUTTER_ACTOR(w));
      clutter_actor_show_all (CLUTTER_ACTOR(urgent));
    }

  g_signal_emit (urgent, urgent_signals[SYNC_INPUT_REGION], 0);
}


static void
on_notification_closed (MoblinNetbookNotifyStore *store, 
                        guint id, 
                        guint reason, 
                        MnbNotificationUrgent *urgent)
{
  MnbNotificationUrgentPrivate *priv = GET_PRIVATE (urgent);
  NbtkWidget *w;

  w = find_widget (priv->notifiers, id);

  if (w)
    {
      if (w == priv->active)
        priv->active = NULL;

      priv->n_notifiers--;
      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->notifiers), 
                                      CLUTTER_ACTOR(w));

      if (priv->active == NULL && priv->n_notifiers > 0)
        {
          priv->active = NBTK_WIDGET (
            clutter_group_get_nth_child (CLUTTER_GROUP (priv->notifiers), 
                                         0));
          clutter_actor_show (CLUTTER_ACTOR(priv->active));
        }
      else
        {
          clutter_actor_hide (CLUTTER_ACTOR(urgent));
          clutter_actor_hide (CLUTTER_ACTOR(priv->notifiers));
        }

      g_signal_emit (urgent, urgent_signals[SYNC_INPUT_REGION], 0);
    }
}

void
mnb_notification_urgent_set_store (MnbNotificationUrgent    *self,
                                   MoblinNetbookNotifyStore *notify_store)
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
mnb_notification_urgent_init (MnbNotificationUrgent *self)
{
  MnbNotificationUrgentPrivate *priv = GET_PRIVATE (self);

  priv->notifiers = CLUTTER_GROUP(clutter_group_new ());

  clutter_actor_set_parent (CLUTTER_ACTOR(priv->notifiers), 
                            CLUTTER_ACTOR(self));

  clutter_actor_hide (CLUTTER_ACTOR(priv->notifiers));

  clutter_actor_set_reactive (CLUTTER_ACTOR(priv->notifiers), TRUE);
  clutter_actor_set_reactive (CLUTTER_ACTOR(self), TRUE);
}

ClutterActor*
mnb_notification_urgent_new (void)
{
  return g_object_new (MNB_TYPE_NOTIFICATION_URGENT, NULL);
}

