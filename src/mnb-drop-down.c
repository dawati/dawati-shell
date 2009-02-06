/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Thomas Wood <thomas@linux.intel.com>
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

#include "mnb-drop-down.h"

#define SLIDE_DURATION 150

G_DEFINE_TYPE (MnbDropDown, mnb_drop_down, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_DROP_DOWN, MnbDropDownPrivate))

enum
{
  SHOW_COMPLETED,

  LAST_SIGNAL
};

static guint dropdown_signals[LAST_SIGNAL] = { 0 };

struct _MnbDropDownPrivate {
    ClutterActor *child;
    NbtkButton *button;
    gint x;
    gint y;
    gint width;
    gint height;
};

static void
mnb_drop_down_get_property (GObject *object, guint property_id,
                            GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_drop_down_set_property (GObject *object, guint property_id,
                            const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_drop_down_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnb_drop_down_parent_class)->dispose (object);
}

static void
mnb_drop_down_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_drop_down_parent_class)->finalize (object);
}

static void
mnb_drop_down_show_completed_cb (ClutterTimeline *timeline, ClutterActor *actor)
{
  g_signal_emit (actor, dropdown_signals[SHOW_COMPLETED], 0);
}

static void
mnb_drop_down_show (ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;
  ClutterTimeline *timeline;
  gint x, y, height, width;
  CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class)->show (actor);
  ClutterAnimation *animation;

  clutter_actor_get_position (actor, &x, &y);
  clutter_actor_get_size (actor, &width, &height);

  /* save the size/position so we can clip while we are moving */
  priv->x = x;
  priv->y = y;
  priv->width = width;
  priv->height = height;


  clutter_actor_set_position (actor, x, -height);
  animation = clutter_actor_animate (actor, CLUTTER_EASE_IN_SINE,
                                     SLIDE_DURATION,
                                     "x", x,
                                     "y", y,
                                     NULL);

  g_signal_connect (clutter_animation_get_timeline (animation),
                    "completed",
                    G_CALLBACK (mnb_drop_down_show_completed_cb),
                    actor);

}

static void
mnb_drop_down_hide_completed_cb (ClutterTimeline *timeline, ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;

  /* the hide animation has finished, so now really hide the actor */
  CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class)->hide (actor);

  /* now that it's hidden we can put it back to where it is suppoed to be */
  clutter_actor_set_position (actor, priv->x, priv->y);
}

static void
mnb_drop_down_hide (ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;
  ClutterAnimation *animation;

  /* de-activate the button */
  if (priv->button)
    {
      /* hide is hooked into the notify::active signal from the button, so
       * make sure we don't get into a loop by checking active first
       */
      if (nbtk_button_get_active (priv->button))
        nbtk_button_set_active (priv->button, FALSE);
    }

  animation = clutter_actor_animate (actor, CLUTTER_EASE_IN_SINE,
                                     SLIDE_DURATION,
                                     "y", -priv->height,
                                     NULL);

  g_signal_connect (clutter_animation_get_timeline (animation),
                    "completed",
                    G_CALLBACK (mnb_drop_down_hide_completed_cb),
                    actor);
}

static void
mnb_drop_down_paint (ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;
  ClutterGeometry geom;

  clutter_actor_get_allocation_geometry (actor, &geom);

  cogl_clip_set (CLUTTER_INT_TO_FIXED (priv->x - geom.x),
                 CLUTTER_INT_TO_FIXED (priv->y - geom.y),
                 CLUTTER_INT_TO_FIXED (priv->width),
                 CLUTTER_INT_TO_FIXED (priv->height));

  CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class)->paint (actor);
  cogl_clip_unset ();
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
mnb_button_toggled_cb (NbtkWidget  *button,
                       GParamSpec  *pspec,
                       MnbDropDown *drop_down)
{
  if (nbtk_button_get_active (NBTK_BUTTON (button)))
    clutter_actor_show (CLUTTER_ACTOR (drop_down));
  else
    clutter_actor_hide (CLUTTER_ACTOR (drop_down));
}

static void
mnb_drop_down_class_init (MnbDropDownClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *clutter_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbDropDownPrivate));

  object_class->get_property = mnb_drop_down_get_property;
  object_class->set_property = mnb_drop_down_set_property;
  object_class->dispose = mnb_drop_down_dispose;
  object_class->finalize = mnb_drop_down_finalize;

  clutter_class->show = mnb_drop_down_show;
  clutter_class->hide = mnb_drop_down_hide;
  clutter_class->paint = mnb_drop_down_paint;
  clutter_class->button_press_event = mnb_button_event_capture;
  clutter_class->button_release_event = mnb_button_event_capture;

  dropdown_signals[SHOW_COMPLETED] =
    g_signal_new ("show-completed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbDropDownClass, show_completed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
mnb_drop_down_init (MnbDropDown *self)
{
  NbtkWidget *footer, *up_button;
  NbtkPadding padding = MNB_PADDING (4, 4, 4, 4);
  ClutterTimeline *timeline;
  MnbDropDownPrivate *priv;

  priv = self->priv = GET_PRIVATE (self);

  /* footer with "up" button */
  footer = nbtk_table_new ();
  nbtk_widget_set_padding (footer, &padding);
  nbtk_widget_set_style_class_name (footer, "drop-down-footer");

  up_button = nbtk_button_new ();
  nbtk_widget_set_style_class_name (up_button, "drop-down-up-button");
  nbtk_table_add_actor (NBTK_TABLE (footer), CLUTTER_ACTOR (up_button), 0, 0);
  clutter_actor_set_size (CLUTTER_ACTOR (up_button), 23, 21);
  clutter_container_child_set (CLUTTER_CONTAINER (footer),
                               CLUTTER_ACTOR (up_button),
                               "keep-aspect-ratio", TRUE,
                               "x-align", 1.0,
                               NULL);
  g_signal_connect_swapped (up_button, "clicked",
                            G_CALLBACK (clutter_actor_hide), self);

  nbtk_table_add_widget (NBTK_TABLE (self), footer, 1, 0);


  timeline = clutter_timeline_new_for_duration (SLIDE_DURATION);

  g_object_set (self,
                "show-on-set-parent", FALSE,
                "reactive", TRUE,
                NULL);
}

NbtkWidget*
mnb_drop_down_new (void)
{
  return g_object_new (MNB_TYPE_DROP_DOWN, NULL);
}

void
mnb_drop_down_set_child (MnbDropDown *drop_down,
                         ClutterActor *child)
{
  g_return_if_fail (MNB_IS_DROP_DOWN (drop_down));
  g_return_if_fail (child == NULL || CLUTTER_IS_ACTOR (child));

  if (drop_down->priv->child)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (drop_down),
                                      drop_down->priv->child);
    }

  if (child)
    nbtk_table_add_actor (NBTK_TABLE (drop_down), child, 0, 0);

  drop_down->priv->child = child;
}

ClutterActor*
mnb_drop_down_get_child (MnbDropDown *drop_down)
{
  g_return_if_fail (MNB_DROP_DOWN (drop_down));

  return drop_down->priv->child;
}

void
mnb_drop_down_set_button (MnbDropDown *drop_down,
                          NbtkButton *button)
{

  g_return_if_fail (MNB_IS_DROP_DOWN (drop_down));
  g_return_if_fail (NBTK_IS_BUTTON (button));

  drop_down->priv->button = button;

  g_signal_connect (button,
                    "notify::active",
                    G_CALLBACK (mnb_button_toggled_cb),
                    drop_down);

}
