/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-toolbar-button.c */
/*
 * Copyright (c) 2009 Intel Corp.
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

#include "mnb-toolbar-button.h"

G_DEFINE_TYPE (MnbToolbarButton, mnb_toolbar_button, NBTK_TYPE_BUTTON)


#define MNB_TOOLBAR_BUTTON_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_TOOLBAR_BUTTON, MnbToolbarButtonPrivate))

struct _MnbToolbarButtonPrivate
{
  ClutterGeometry pick;
  ClutterActor  *old_bg;
};

static void
mnb_toolbar_button_dispose (GObject *object)
{
  MnbToolbarButtonPrivate *priv = MNB_TOOLBAR_BUTTON (object)->priv;

  if (priv->old_bg)
    {
      clutter_actor_unparent (priv->old_bg);
      priv->old_bg = NULL;
    }
}

static void
mnb_toolbar_button_pick (ClutterActor *actor, const ClutterColor *pick_color)
{
  MnbToolbarButtonPrivate *priv = MNB_TOOLBAR_BUTTON (actor)->priv;

  cogl_set_source_color4ub (pick_color->red,
                            pick_color->green,
                            pick_color->blue,
                            pick_color->alpha);

  cogl_rectangle (priv->pick.x,
                  priv->pick.y,
                  priv->pick.width,
                  priv->pick.height);

  CLUTTER_ACTOR_CLASS (mnb_toolbar_button_parent_class)->pick (actor, pick_color);
}

static void
mnb_toolbar_button_map (ClutterActor *actor)
{
  MnbToolbarButtonPrivate *priv = MNB_TOOLBAR_BUTTON (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_toolbar_button_parent_class)->map (actor);

  if (priv->old_bg)
    clutter_actor_map (priv->old_bg);
}

static void
mnb_toolbar_button_unmap (ClutterActor *actor)
{
  MnbToolbarButtonPrivate *priv = MNB_TOOLBAR_BUTTON (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_toolbar_button_parent_class)->unmap (actor);

  if (priv->old_bg)
    clutter_actor_unmap (priv->old_bg);
}

static void
mnb_toolbar_button_paint_background (NbtkWidget         *actor,
                                     ClutterActor       *background,
                                     const ClutterColor *color)
{
  MnbToolbarButtonPrivate *priv = MNB_TOOLBAR_BUTTON (actor)->priv;

  NBTK_WIDGET_CLASS (
         mnb_toolbar_button_parent_class)->draw_background (actor,
                                                            background,
                                                            color);

  if (priv->old_bg)
    clutter_actor_paint (priv->old_bg);
}

static void
mnb_toolbar_button_transition (NbtkButton *button, ClutterActor *old_bg)
{
  MnbToolbarButtonPrivate *priv = MNB_TOOLBAR_BUTTON (button)->priv;
  const gchar *pseudo_class;
  gint duration;
  ClutterActor *bg_image;
  ClutterActor *icon;

  pseudo_class = nbtk_stylable_get_pseudo_class (NBTK_STYLABLE (button));

  if (priv->old_bg)
    {
      clutter_actor_unparent (priv->old_bg);
      priv->old_bg = NULL;
    }

  bg_image = nbtk_widget_get_border_image (NBTK_WIDGET (button));
  if (!bg_image)
    return;

  icon = nbtk_widget_get_background_image (NBTK_WIDGET (button));
  if (icon)
    g_object_set (G_OBJECT (icon),
                  "scale-gravity", CLUTTER_GRAVITY_CENTER,
                  NULL);

  g_object_get (button, "transition-duration", &duration, NULL);

  if (!g_strcmp0 (pseudo_class, "hover"))
    {
      /* bounce the (semi-transparent) background and icon */
      clutter_actor_set_opacity (bg_image, 0x26);
      clutter_actor_set_scale_with_gravity (bg_image, 0.5, 0.5,
                                            CLUTTER_GRAVITY_CENTER);

      clutter_actor_animate (bg_image, CLUTTER_EASE_OUT_ELASTIC,
                             duration,
                             "scale-x", 1.0,
                             "scale-y", 1.0,
                             NULL);

      if (icon)
        {
          clutter_actor_set_scale_with_gravity (icon, 0.5, 0.5,
                                                CLUTTER_GRAVITY_CENTER);

          clutter_actor_animate (icon, CLUTTER_EASE_OUT_ELASTIC,
                                 duration * 1.5,
                                 "scale-x", 1.0,
                                 "scale-y", 1.0,
                                 NULL);
        }
    }
  else if (!g_strcmp0 (pseudo_class, "active"))
    {
      /* shrink the background and the icon */
      if (icon)
        clutter_actor_set_scale_with_gravity (icon, 1.0, 1.0,
                                              CLUTTER_GRAVITY_CENTER);
      clutter_actor_set_scale_with_gravity (bg_image, 1.0, 1.0,
                                            CLUTTER_GRAVITY_CENTER);
      clutter_actor_set_opacity (bg_image, 0x26);
      clutter_actor_animate (bg_image, CLUTTER_LINEAR,
                             150,
                             "opacity", 0xff,
                             "scale-x", 0.8,
                             "scale-y", 0.8,
                             NULL);
      if (icon)
        clutter_actor_animate (icon, CLUTTER_LINEAR,
                               150,
                               "scale-x", 0.7,
                               "scale-y", 0.7,
                               NULL);
    }
  else if (!g_strcmp0 (pseudo_class, "checked"))
    {
      /* - restore the icon and old (hover) background to full size
       * - fade in new background */
      if (old_bg)
        {
          priv->old_bg = old_bg;
          clutter_actor_set_parent (old_bg, CLUTTER_ACTOR (button));
          clutter_actor_set_scale_with_gravity (old_bg, 0.8, 0.8,
                                                CLUTTER_GRAVITY_CENTER);
          clutter_actor_animate (old_bg, CLUTTER_LINEAR,
                                 150,
                                 "scale-x", 1.0,
                                 "scale-y", 1.0,
                                 NULL);
        }

      clutter_actor_set_opacity (bg_image, 0x0);
      clutter_actor_animate (bg_image, CLUTTER_EASE_IN_EXPO,
                             150,
                             "opacity", 0xff,
                             NULL);

      if (icon)
        {
          clutter_actor_set_scale (icon, 0.8, 0.8);
          clutter_actor_animate (icon, CLUTTER_EASE_OUT_BACK,
                                 150,
                                 "scale-x", 1.0,
                                 "scale-y", 1.0,
                                 NULL);
        }
    }
}

static gboolean
mnb_toolbar_button_press (ClutterActor *actor, ClutterButtonEvent *event)
{
#if 0
  /* Disable until a more complete solution is ready */
  /* don't react to button press when already active */
  if (nbtk_button_get_checked (NBTK_BUTTON (actor)))
    return TRUE;
  else
#endif
    return CLUTTER_ACTOR_CLASS (mnb_toolbar_button_parent_class)->button_press_event (actor,
                                                                       event);
}

static gboolean
mnb_toolbar_button_enter (ClutterActor *actor, ClutterCrossingEvent *event)
{
  /* don't show a tooltip when the button is "checked" */
  if (nbtk_button_get_checked (NBTK_BUTTON (actor)))
    return TRUE;
  else
    return CLUTTER_ACTOR_CLASS (mnb_toolbar_button_parent_class)->enter_event (actor,
                                                                             event);
}

static void
mnb_toolbar_button_allocate (ClutterActor          *actor,
                             const ClutterActorBox *box,
                             gboolean               origin_changed)
{
  MnbToolbarButtonPrivate *priv = MNB_TOOLBAR_BUTTON (actor)->priv;

  CLUTTER_ACTOR_CLASS (
             mnb_toolbar_button_parent_class)->allocate (actor,
                                                         box,
                                                         origin_changed);

  if (priv->old_bg)
    {
      ClutterActorBox frame_box = {
          0, 0, box->x2 - box->x1, box->y2 - box->y1
      };

      clutter_actor_allocate (priv->old_bg,
                              &frame_box,
                              origin_changed);
    }
}

static void
mnb_toolbar_button_class_init (MnbToolbarButtonClass *klass)
{
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  NbtkWidgetClass *widget_class  = NBTK_WIDGET_CLASS (klass);
  NbtkButtonClass *button_class  = NBTK_BUTTON_CLASS (klass);
  GObjectClass    *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbToolbarButtonPrivate));

  actor_class->allocate           = mnb_toolbar_button_allocate;
  actor_class->pick               = mnb_toolbar_button_pick;
  actor_class->button_press_event = mnb_toolbar_button_press;
  actor_class->enter_event        = mnb_toolbar_button_enter;
  actor_class->map                = mnb_toolbar_button_map;
  actor_class->unmap              = mnb_toolbar_button_unmap;

  button_class->transition        = mnb_toolbar_button_transition;

  gobject_class->dispose          = mnb_toolbar_button_dispose;

  widget_class->draw_background   = mnb_toolbar_button_paint_background;
}

static void
mnb_toolbar_button_init (MnbToolbarButton *self)
{
  self->priv = MNB_TOOLBAR_BUTTON_GET_PRIVATE (self);

  g_object_set (self, "transition-duration", 500, NULL);
}

NbtkWidget*
mnb_toolbar_button_new (void)
{
  return g_object_new (MNB_TYPE_TOOLBAR_BUTTON, NULL);
}

void
mnb_toolbar_button_set_reactive_area (MnbToolbarButton *button,
                                      gint              x,
                                      gint              y,
                                      gint              width,
                                      gint              height)
{
  MnbToolbarButtonPrivate *priv = MNB_TOOLBAR_BUTTON (button)->priv;

  priv->pick.x = x;
  priv->pick.y = y;
  priv->pick.width = width;
  priv->pick.height = height;
}
