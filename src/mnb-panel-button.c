/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-panel-button.c */
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

#include "mnb-panel-button.h"

G_DEFINE_TYPE (MnbPanelButton, mnb_panel_button, NBTK_TYPE_BUTTON)


#define MNB_PANEL_BUTTON_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_PANEL_BUTTON, MnbPanelButtonPrivate))

struct _MnbPanelButtonPrivate
{
  ClutterGeometry pick;
};


static void
mnb_panel_button_pick (ClutterActor       *actor,
                       const ClutterColor *pick_color)
{
  MnbPanelButtonPrivate *priv = MNB_PANEL_BUTTON (actor)->priv;

  cogl_set_source_color4ub (pick_color->red,
                            pick_color->green,
                            pick_color->blue,
                            pick_color->alpha);

  cogl_rectangle (priv->pick.x,
                  priv->pick.y,
                  priv->pick.width,
                  priv->pick.height);

  CLUTTER_ACTOR_CLASS (mnb_panel_button_parent_class)->pick (actor, pick_color);
}

static gboolean
mnb_panel_button_transition (NbtkButton *button, ClutterActor *old_bg)
{
  const gchar *pseudo_class;
  gint duration;
  ClutterActor *bg_image;
  ClutterActor *icon;
  NbtkButtonPrivate *priv = button->priv;

  pseudo_class = nbtk_stylable_get_pseudo_class (NBTK_STYLABLE (button));

  bg_image = nbtk_widget_get_border_image (NBTK_WIDGET (button));
  if (!bg_image)
    return TRUE;

  icon = nbtk_widget_get_background_image (NBTK_WIDGET (button));
  g_object_set (G_OBJECT (icon),
                "scale-gravity", CLUTTER_GRAVITY_CENTER,
                NULL);
  if (!icon)
    return TRUE; /* no icon? this surely isn't a real panel button! */

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

      clutter_actor_set_scale_with_gravity (icon, 0.5, 0.5,
                                            CLUTTER_GRAVITY_CENTER);

      clutter_actor_animate (icon, CLUTTER_EASE_OUT_ELASTIC,
                             duration * 1.5,
                             "scale-x", 1.0,
                             "scale-y", 1.0,
                             NULL);
    }
  else if (!g_strcmp0 (pseudo_class, "active"))
    {
      /* shrink the background and the icon */
      clutter_actor_set_scale_with_gravity (icon, 1.0, 1.0,
                                            CLUTTER_GRAVITY_CENTER);
      clutter_actor_set_scale_with_gravity (bg_image, 1.0, 1.0,
                                            CLUTTER_GRAVITY_CENTER);
      clutter_actor_set_opacity (bg_image, 0x26);
      if (old_bg)
        clutter_actor_set_opacity (old_bg, 0x0);
      clutter_actor_animate (bg_image, CLUTTER_LINEAR,
                             150,
                             "opacity", 0xff,
                             "scale-x", 0.8,
                             "scale-y", 0.8,
                             NULL);
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

      clutter_actor_set_scale (icon, 0.8, 0.8);
      clutter_actor_animate (icon, CLUTTER_EASE_OUT_BACK,
                             150,
                             "scale-x", 1.0,
                             "scale-y", 1.0,
                             NULL);
      return FALSE;
    }

  return TRUE;

}

static gboolean
mnb_panel_button_press (ClutterActor       *actor,
                        ClutterButtonEvent *event)
{
#if 0
  /* Disable until a more complete solution is ready */
  /* don't react to button press when already active */
  if (nbtk_button_get_checked (NBTK_BUTTON (actor)))
    return TRUE;
  else
#endif
    return CLUTTER_ACTOR_CLASS (mnb_panel_button_parent_class)->button_press_event (actor,
                                                                       event);
}

static gboolean
mnb_panel_button_enter (ClutterActor         *actor,
                        ClutterCrossingEvent *event)
{
  /* don't show a tooltip when the button is "checked" */
  if (nbtk_button_get_checked (NBTK_BUTTON (actor)))
    return TRUE;
  else
    return CLUTTER_ACTOR_CLASS (mnb_panel_button_parent_class)->enter_event (actor,
                                                                             event);
}

static void
mnb_panel_button_class_init (MnbPanelButtonClass *klass)
{
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  NbtkButtonClass *button_class = NBTK_BUTTON_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbPanelButtonPrivate));

  actor_class->pick = mnb_panel_button_pick;
  actor_class->button_press_event = mnb_panel_button_press;
  actor_class->enter_event = mnb_panel_button_enter;

  button_class->transition = mnb_panel_button_transition;
}

static void
mnb_panel_button_init (MnbPanelButton *self)
{
  self->priv = MNB_PANEL_BUTTON_GET_PRIVATE (self);

  g_object_set (self, "transition-duration", 500, NULL);
}

NbtkWidget*
mnb_panel_button_new (void)
{
  return g_object_new (MNB_TYPE_PANEL_BUTTON, NULL);
}

void
mnb_panel_button_set_reactive_area (MnbPanelButton  *button,
                                    gint             x,
                                    gint             y,
                                    gint             width,
                                    gint             height)
{
  MnbPanelButtonPrivate *priv = MNB_PANEL_BUTTON (button)->priv;

  priv->pick.x = x;
  priv->pick.y = y;
  priv->pick.width = width;
  priv->pick.height = height;
}
