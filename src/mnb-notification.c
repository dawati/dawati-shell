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

#include "mnb-notification.h"
#include "moblin-netbook-notify-store.h"

#define SLIDE_DURATION 150

G_DEFINE_TYPE (MnbNotification, mnb_notification, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_NOTIFICATION, MnbNotificationPrivate))

enum {
  CLOSED,
  N_SIGNALS,
};


enum {
  PROP_0,

  PROP_ICON,
  PROP_TEXT
};

static guint signals[N_SIGNALS] = {0};

struct _MnbNotificationPrivate {
  NbtkWidget   *label;
  ClutterActor *icon;
  NbtkWidget   *dismiss_button;
  NbtkWidget   *action_button;

  gchar        *app_name;
  guint         id;
  gchar        *summary;
  gchar        *body;
  gchar       **actions; 
  GHashTable   *hints;
  gint          timeout;
};

static void
mnb_notification_get_property (GObject *object, guint property_id,
                            GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_notification_set_property (GObject *object, guint property_id,
                               const GValue *value, GParamSpec *pspec)
{
  switch (property_id) 
    {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_notification_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnb_notification_parent_class)->dispose (object);
}

static void
mnb_notification_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_notification_parent_class)->finalize (object);
}

static void
mnb_notification_show_completed_cb (ClutterActor *actor, gpointer data)
{
#if 0
  g_signal_emit (actor, notification_signals[SHOW_COMPLETED], 0);
#endif
}

static void
mnb_notification_show (ClutterActor *actor)
{
#if 0
  MnbNotificationPrivate *priv = MNB_NOTIFICATION (actor)->priv;
  ClutterTimeline *timeline;
  gint x, y, height, width;
  CLUTTER_ACTOR_CLASS (mnb_notification_parent_class)->show (actor);

  clutter_actor_get_position (actor, &x, &y);
  clutter_actor_get_size (actor, &width, &height);

  /* save the size/position so we can clip while we are moving */
  priv->x = x;
  priv->y = y;
  priv->width = width;
  priv->height = height;


  clutter_actor_set_position (actor, x, -height);
  clutter_effect_move (MNB_NOTIFICATION (actor)->priv->slide_effect,
                       actor,
                       x,
                       y,
                       mnb_notification_show_completed_cb, NULL);
#endif
}

static void
mnb_notification_hide_completed_cb (ClutterActor *actor, gpointer data)
{
  MnbNotificationPrivate *priv = MNB_NOTIFICATION (actor)->priv;
#if 0
  /* the hide animation has finished, so now really hide the actor */
  CLUTTER_ACTOR_CLASS (mnb_notification_parent_class)->hide (actor);

  /* now that it's hidden we can put it back to where it is suppoed to be */
  clutter_actor_set_position (actor, priv->x, priv->y);
#endif
}

static void
mnb_notification_hide (ClutterActor *actor)
{
  MnbNotificationPrivate *priv = MNB_NOTIFICATION (actor)->priv;

#if 0
  /* de-activate the button */
  if (priv->button)
    {
      /* hide is hooked into the notify::active signal from the button, so
       * make sure we don't get into a loop by checking active first
       */
      if (nbtk_button_get_active (priv->button))
        nbtk_button_set_active (priv->button, FALSE);
    }

  clutter_effect_move (MNB_NOTIFICATION (actor)->priv->slide_effect,
                       actor,
                       priv->x,
                       -priv->height,
                       mnb_notification_hide_completed_cb, NULL);
#endif
}

static gboolean
mnb_button_event_capture (ClutterActor *actor, ClutterButtonEvent *event)
{
  /* prevent the event from moving up the scene graph, since we don't want
   * any events to accidently fall onto application windows below the
   * drop down.
   */
  return TRUE;
}

static void
mnb_notification_class_init (MnbNotificationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *clutter_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbNotificationPrivate));

  object_class->get_property = mnb_notification_get_property;
  object_class->set_property = mnb_notification_set_property;
  object_class->dispose = mnb_notification_dispose;
  object_class->finalize = mnb_notification_finalize;

  signals[CLOSED] 
    = g_signal_new ("closed",
                    G_OBJECT_CLASS_TYPE (klass),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (MnbNotificationClass, closed),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);


#if 0
  clutter_class->show = mnb_notification_show;
  clutter_class->hide = mnb_notification_hide;

  clutter_class->button_press_event = mnb_button_event_capture;
  clutter_class->button_release_event = mnb_button_event_capture;

  notification_signals[SHOW_COMPLETED] =
    g_signal_new ("show-completed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbNotificationClass, show_completed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
#endif
}

/* FIXME, this is duped from drop-down.h, should be global ? */
#define MNB_PADDING(a, b, c, d) {CLUTTER_UNITS_FROM_INT (a), CLUTTER_UNITS_FROM_INT (b), \
                                 CLUTTER_UNITS_FROM_INT (c), CLUTTER_UNITS_FROM_INT (d) }


static void
mnb_notification_init (MnbNotification *self)
{
  NbtkWidget *footer, *up_button;
  NbtkPadding padding = MNB_PADDING (4, 4, 4, 4);
  ClutterTimeline *timeline;
  MnbNotificationPrivate *priv;

  priv = self->priv = GET_PRIVATE (self);

  // nbtk_widget_set_padding (NBTK_WIDGET (self), padding);
  nbtk_widget_set_style_class_name (NBTK_WIDGET (self), "notification");

  priv->dismiss_button = nbtk_button_new ();
  priv->action_button  = nbtk_button_new ();
  //priv->icon           = clutter_texture_new ();
  priv->label          = nbtk_label_new ("");

  //clutter_label_set_use_markup (CLUTTER_LABEL(priv->label),TRUE);

  nbtk_table_add_widget (NBTK_TABLE (self), priv->label, 0, 0);
  nbtk_table_set_widget_colspan (NBTK_TABLE (self), priv->label, 2);

  nbtk_button_set_label (NBTK_BUTTON (priv->dismiss_button), "Dismiss");
  nbtk_table_add_widget (NBTK_TABLE (self), priv->dismiss_button, 1, 1);

  //nbtk_widget_set_style_class_name (priv->label, "notification-label");

#if 0
  clutter_actor_set_size (CLUTTER_ACTOR (up_button), 23, 21);

  clutter_container_child_set (CLUTTER_CONTAINER (footer),
                               CLUTTER_ACTOR (up_button),
                               "keep-aspect-ratio", TRUE,
                               "x-align", 1.0,
                               NULL);
#endif
}

NbtkWidget*
mnb_notification_new (void)
{
  return g_object_new (MNB_TYPE_NOTIFICATION,
                       "show-on-set-parent", FALSE,
                       "reactive", TRUE,
                       NULL);
}

void
mnb_notification_update (MnbNotification *notification,
                         Notification    *details)
{
  MnbNotificationPrivate *priv;

  g_return_if_fail (MNB_IS_NOTIFICATION (notification));

  priv = GET_PRIVATE (notification);

  priv->id = details->id;

  nbtk_label_set_text (NBTK_LABEL(priv->label), details->body);

  printf("setting label details to '%s'\n", details->body);
}

guint
mnb_notification_get_id (MnbNotification *notification)
{
  MnbNotificationPrivate *priv;

  g_return_if_fail (MNB_IS_NOTIFICATION (notification));

  priv = GET_PRIVATE (notification);

  return priv->id;
}

