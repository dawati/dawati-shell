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
#define MNB_PADDING(a, b, c, d) {CLUTTER_UNITS_FROM_INT (a), CLUTTER_UNITS_FROM_INT (b), \
                                 CLUTTER_UNITS_FROM_INT (c), CLUTTER_UNITS_FROM_INT (d) }

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
    ClutterEffectTemplate *slide_effect;
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
mnb_drop_down_show_completed_cb (ClutterActor *actor, gpointer data)
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

  clutter_actor_get_position (actor, &x, &y);
  clutter_actor_get_size (actor, &width, &height);

  /* save the size/position so we can clip while we are moving */
  priv->x = x;
  priv->y = y;
  priv->width = width;
  priv->height = height;


  clutter_actor_set_position (actor, x, -height);
  clutter_actor_show (actor);
  clutter_effect_move (MNB_DROP_DOWN (actor)->priv->slide_effect,
                       actor,
                       x,
                       y,
                       mnb_drop_down_show_completed_cb, NULL);
}

static void
mnb_drop_down_hide (ClutterActor *actor)
{
  /* may have a different effect in the future */
  CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class)->hide (actor);
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

  nbtk_widget_set_style_class_name (NBTK_WIDGET (self), "drop-down-background");

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
  priv->slide_effect = clutter_effect_template_new (timeline,
                                                    CLUTTER_ALPHA_SINE_INC);
}

NbtkWidget*
mnb_drop_down_new (void)
{
  return g_object_new (MNB_TYPE_DROP_DOWN,
                       "show-on-set-parent", FALSE,
                       "reactive", TRUE,
                       NULL);
}

void
mnb_drop_down_set_child (MnbDropDown *drop_down,
                         ClutterActor *child)
{
  g_return_if_fail (MNB_IS_DROP_DOWN (drop_down));
  g_return_if_fail (CLUTTER_IS_ACTOR (child));

  if (drop_down->priv->child)
    {
      clutter_container_remove (CLUTTER_CONTAINER (drop_down), child);
    }

  nbtk_table_add_actor (NBTK_TABLE (drop_down), child, 0, 0);
  drop_down->priv->child = child;
}

ClutterActor*
mnb_drop_down_get_child (MnbDropDown *drop_down)
{
  g_return_if_fail (MNB_DROP_DOWN (drop_down));

  return drop_down->priv->child;
}
