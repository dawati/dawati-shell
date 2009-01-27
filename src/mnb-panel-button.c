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
  gint trans_x, trans_y;

  cogl_color (pick_color);

  cogl_rectangle (priv->pick.x,
                  priv->pick.y,
                  priv->pick.width,
                  priv->pick.height);

  CLUTTER_ACTOR_CLASS (mnb_panel_button_parent_class)->pick (actor, pick_color);
}


static void
mnb_panel_button_class_init (MnbPanelButtonClass *klass)
{
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbPanelButtonPrivate));

  actor_class->pick = mnb_panel_button_pick;
}

static void
mnb_panel_button_init (MnbPanelButton *self)
{
  self->priv = MNB_PANEL_BUTTON_GET_PRIVATE (self);
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
