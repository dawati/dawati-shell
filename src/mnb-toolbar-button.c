/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-toolbar-button.c */
/*
 * Copyright (c) 2009, 2010 Intel Corp.
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
#include "mnb-toolbar.h"

#define TRANSITION_DURATION 500

G_DEFINE_TYPE (MnbToolbarButton, mnb_toolbar_button, MX_TYPE_BUTTON)


#define MNB_TOOLBAR_BUTTON_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_TOOLBAR_BUTTON, MnbToolbarButtonPrivate))

struct _MnbToolbarButtonPrivate
{
  ClutterGeometry pick;
};

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

  CLUTTER_ACTOR_CLASS (mnb_toolbar_button_parent_class)->pick (actor,
                                                               pick_color);
}

static void
mnb_toolbar_button_transition (MnbToolbarButton *button)
{
  const gchar  *pseudo_class;
  ClutterActor *bg_image;
  ClutterActor *icon;

  pseudo_class = mx_stylable_get_style_pseudo_class (MX_STYLABLE (button));

  bg_image = mx_widget_get_border_image (MX_WIDGET (button));

  if (!bg_image)
    return;

  icon = mx_widget_get_background_image (MX_WIDGET (button));

  if (!icon)
    icon = mx_bin_get_child (MX_BIN (button));

  if (icon)
    g_object_set (G_OBJECT (icon),
                  "scale-gravity", CLUTTER_GRAVITY_CENTER,
                  NULL);

  if (!pseudo_class)
    {
      if (icon)
        clutter_actor_animate (icon, CLUTTER_LINEAR,
                               150,
                               "scale-x", 1.0,
                               "scale-y", 1.0,
                               NULL);

    }
  else if (mx_stylable_style_pseudo_class_contains (MX_STYLABLE (button),
                                                    "hover"))
    {
      if (icon)
        {
          clutter_actor_set_scale_with_gravity (icon, 0.5, 0.5,
                                                CLUTTER_GRAVITY_CENTER);

          clutter_actor_animate (icon, CLUTTER_EASE_OUT_ELASTIC,
                                 TRANSITION_DURATION * 1.5,
                                 "scale-x", 1.0,
                                 "scale-y", 1.0,
                                 NULL);
        }
    }
  else if (mx_stylable_style_pseudo_class_contains (MX_STYLABLE (button),
                                                    "active"))
    {
      /* shrink the background and the icon */
      if (icon)
        {
          clutter_actor_set_scale_with_gravity (icon, 1.0, 1.0,
                                                CLUTTER_GRAVITY_CENTER);
          clutter_actor_animate (icon, CLUTTER_LINEAR,
                                 150,
                                 "scale-x", 0.7,
                                 "scale-y", 0.7,
                                 NULL);
        }
    }
  /*
   * NB: we do not animate the change to checked because the difference in
   *     colour between the hover and the checked backgrounds makes it look
   *     broken.
   */
}

static gboolean
mnb_toolbar_button_press (ClutterActor *actor, ClutterButtonEvent *event)
{
  ClutterActor *toolbar = actor;

  /*
   * Block any key presses while the Toolbar is waiting for a panel to show.
   */
  while ((toolbar = clutter_actor_get_parent (toolbar)) &&
         !MNB_IS_TOOLBAR (toolbar));

  g_assert (toolbar);

  if (mnb_toolbar_is_waiting_for_panel (MNB_TOOLBAR (toolbar)))
    return TRUE;

  return CLUTTER_ACTOR_CLASS (
           mnb_toolbar_button_parent_class)->button_press_event (actor, event);
}

static gboolean
mnb_toolbar_button_enter (ClutterActor *actor, ClutterCrossingEvent *event)
{
  /* don't show a tooltip when the button is "checked" */
  if (mx_button_get_toggled (MX_BUTTON (actor)))
    return TRUE;
  else
    return CLUTTER_ACTOR_CLASS (
                 mnb_toolbar_button_parent_class)->enter_event (actor, event);
}

static void
mnb_toolbar_button_hide (ClutterActor *actor)
{
  CLUTTER_ACTOR_CLASS (mnb_toolbar_button_parent_class)->hide (actor);

  /*
   * Clear any left over hover state (MB#5518)
   */
  mx_stylable_set_style_pseudo_class (MX_STYLABLE (actor), NULL);
}

static void
mnb_toolbar_button_get_preferred_width (ClutterActor *self,
                                        gfloat        for_height,
                                        gfloat       *min_width_p,
                                        gfloat       *natural_width_p)
{
  gfloat width;

  width = BUTTON_WIDTH + BUTTON_INTERNAL_PADDING;

  if (min_width_p)
    *min_width_p = width;

  if (natural_width_p)
    *natural_width_p = width;
}

static void
mnb_toolbar_button_get_preferred_height (ClutterActor *self,
                                         gfloat        for_width,
                                         gfloat       *min_height_p,
                                         gfloat       *natural_height_p)
{
  gfloat height;

  height = BUTTON_HEIGHT;

  if (min_height_p)
    *min_height_p = height;

  if (natural_height_p)
    *natural_height_p = height;
}

static void
mnb_toolbar_button_class_init (MnbToolbarButtonClass *klass)
{
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbToolbarButtonPrivate));

  actor_class->pick                 = mnb_toolbar_button_pick;
  actor_class->button_press_event   = mnb_toolbar_button_press;
  actor_class->enter_event          = mnb_toolbar_button_enter;
  actor_class->hide                 = mnb_toolbar_button_hide;
  actor_class->get_preferred_width  = mnb_toolbar_button_get_preferred_width;
  actor_class->get_preferred_height = mnb_toolbar_button_get_preferred_height;
}

static void
mnb_toolbar_button_style_changed (MnbToolbarButton    *button,
                                  MxStyleChangedFlags  flags)
{
  mnb_toolbar_button_transition (button);
}

static void
mnb_toolbar_button_init (MnbToolbarButton *self)
{
  self->priv = MNB_TOOLBAR_BUTTON_GET_PRIVATE (self);

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mnb_toolbar_button_style_changed), NULL);
}

ClutterActor*
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
