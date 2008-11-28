/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *         Thomas Wood <thomas@linux.intel.com>
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

#include "moblin-netbook.h"
#include "moblin-netbook-ui.h"
#include "moblin-netbook-panel.h"
#include "moblin-netbook-launcher.h"

#define BUTTON_WIDTH 66
#define BUTTON_HEIGHT 55
#define BUTTON_SPACING 10

extern MutterPlugin mutter_plugin;
static inline MutterPlugin *
mutter_get_plugin ()
{
  return &mutter_plugin;
}

/*
 * The slide-out top panel.
 */
static void
on_panel_back_effect_complete (ClutterActor *panel, gpointer data)
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;

  priv->panel_back_in_progress = FALSE;

  if (!priv->workspace_chooser && !priv->workspace_switcher)
    disable_stage (plugin, CurrentTime);
}

void
hide_panel ()
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;
  guint          height = clutter_actor_get_height (priv->panel);
  gint           x      = clutter_actor_get_x (priv->panel);

  priv->panel_back_in_progress  = TRUE;

  clutter_effect_move (priv->panel_slide_effect,
                       priv->panel, x, -height,
                       on_panel_back_effect_complete,
                       NULL);
  priv->panel_out = FALSE;
}

static gboolean
workspace_button_input_cb (ClutterActor *actor,
                           ClutterEvent *event,
                           gpointer      data)
{
  show_workspace_switcher ();
  return TRUE;
}

static ClutterActor *
panel_append_toolbar_button (ClutterActor *container,
                             gchar       *name,
                             GCallback    callback)
{
  NbtkWidget *button;
  static int n_buttons = 0;

  button = nbtk_button_new ();
  nbtk_button_set_toggle_mode (NBTK_BUTTON (button), TRUE);
  clutter_actor_set_name (CLUTTER_ACTOR (button), name);
  clutter_actor_set_size (CLUTTER_ACTOR (button), BUTTON_WIDTH, BUTTON_HEIGHT);
  clutter_actor_set_position (CLUTTER_ACTOR (button),
                              213 + (BUTTON_WIDTH * n_buttons) + (BUTTON_SPACING * n_buttons),
                              PANEL_HEIGHT - BUTTON_HEIGHT);

  clutter_container_add_actor (CLUTTER_CONTAINER (container), CLUTTER_ACTOR (button));

  if (callback)
    g_signal_connect (button,
                      "clicked",
                      callback,
                      NULL);

  n_buttons++;
  return button;
}

static gboolean
launcher_button_input_cb (ClutterActor *actor,
                          ClutterEvent *event,
                          gpointer      data)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  clutter_actor_show (priv->launcher);
  return TRUE;
}

ClutterActor *
make_panel (gint width)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;
  ClutterActor  *panel;
  ClutterActor  *background;
  ClutterColor   clr = {0x0, 0x0, 0x0, 0xff};
  ClutterActor  *launcher, *overlay;
  gint           x, w;

  overlay = mutter_plugin_get_overlay_group (plugin);

  panel = clutter_group_new ();

  /* FIME -- size and color */
  background = clutter_rectangle_new_with_color (&clr);
  clutter_container_add_actor (CLUTTER_CONTAINER (panel), background);
  clutter_actor_set_size (background, width, PANEL_HEIGHT);

  panel_append_toolbar_button (panel, "m-space-button", NULL);
  panel_append_toolbar_button (panel, "status-button", NULL);
  panel_append_toolbar_button (panel, "spaces-button", G_CALLBACK (launcher_button_input_cb));
  panel_append_toolbar_button (panel, "internet-button", NULL);
  panel_append_toolbar_button (panel, "media-button", NULL);
  panel_append_toolbar_button (panel, "apps-button", G_CALLBACK (launcher_button_input_cb));
  panel_append_toolbar_button (panel, "people-button", NULL);
  panel_append_toolbar_button (panel, "pasteboard-button", NULL);

  priv->launcher = make_launcher (width);
  clutter_actor_set_y (priv->launcher, clutter_actor_get_height (panel));

  clutter_container_add_actor (CLUTTER_CONTAINER (overlay), priv->launcher);
  clutter_actor_hide (priv->launcher);

  return panel;
}

