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

#include "tidy-behaviour-bounce.h"

#define BUTTON_WIDTH 66
#define BUTTON_HEIGHT 55
#define BUTTON_SPACING 10

extern MutterPlugin mutter_plugin;
static inline MutterPlugin *
mutter_get_plugin ()
{
  return &mutter_plugin;
}

static void toggle_buttons_cb (NbtkButton *button, PluginPrivate *priv);

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

  /* make sure no buttons are 'active' */
  toggle_buttons_cb (NULL, priv);

  priv->panel_out = FALSE;
}

static gboolean
spaces_button_cb (ClutterActor *actor,
                  ClutterEvent *event,
                  gpointer      data)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  /* already showing */
  if (priv->workspace_switcher)
    return TRUE;

  /* No way to know if showing */
  clutter_actor_hide (priv->launcher);
  show_workspace_switcher ();
  return TRUE;
}

static void
toggle_buttons_cb (NbtkButton *button, PluginPrivate *priv)
{
  gint i;

  for (i = 0; i < 8; i++)
    if (priv->panel_buttons[i] != (ClutterActor*)button)
      nbtk_button_set_active (NBTK_BUTTON (priv->panel_buttons[i]), FALSE);
}

static ClutterActor*
panel_append_toolbar_button (ClutterActor  *container,
                             gchar         *name,
                             GCallback      callback,
                             PluginPrivate *data)
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

  g_signal_connect (button, "clicked", G_CALLBACK (toggle_buttons_cb), data);
  if (callback)
    g_signal_connect (button,
                      "clicked",
                      callback,
                      data);

  n_buttons++;
  return CLUTTER_ACTOR (button);
}

static gboolean
launcher_button_cb (ClutterActor *actor,
                    ClutterEvent *event,
                    gpointer      data)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  /* if workspace switcher is showing... HACK */
  if (priv->workspace_switcher)
    {
      clutter_actor_destroy (priv->workspace_switcher);
      priv->workspace_switcher = NULL;
    }

  clutter_actor_move_anchor_point_from_gravity (priv->launcher,
                                                CLUTTER_GRAVITY_CENTER);

  clutter_actor_set_scale (priv->launcher, 0.0, 0.0);
  clutter_actor_show (priv->launcher);

  tidy_bounce_scale (priv->launcher, 200);

  return TRUE;
}

static gboolean 
update_time_date (PluginPrivate *priv)
{
  time_t         t;
  struct tm     *tmp;
  char           time_str[64];

  t = time (NULL);
  tmp = localtime (&t);
  if (tmp)
    strftime (time_str, 64, "%l:%M %P", tmp);
  else
    snprintf (time_str, 64, "Time");
  clutter_label_set_text (CLUTTER_LABEL (priv->panel_time), time_str);

  if (tmp)
    strftime (time_str, 64, "%B %e, %Y", tmp);
  else
    snprintf (time_str, 64, "Date");
  clutter_label_set_text (CLUTTER_LABEL (priv->panel_date), time_str);

  return TRUE;
}

ClutterActor *
make_panel (gint width)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;
  ClutterActor  *panel;
  ClutterActor  *background;
  ClutterColor   clr = {0x0, 0x0, 0x0, 0xce};
  ClutterColor   lbl_clr = {0xc1, 0xc1, 0xc1, 0xff};
  ClutterActor  *launcher, *overlay;
  gint           x, w;

  overlay = mutter_plugin_get_overlay_group (plugin);

  panel = clutter_group_new ();

  /* FIME -- size and color */
  background = clutter_rectangle_new_with_color (&clr);
  clutter_container_add_actor (CLUTTER_CONTAINER (panel), background);
  clutter_actor_set_size (background, width, PANEL_HEIGHT);

  priv->panel_buttons[0] = panel_append_toolbar_button (panel, "m-space-button", NULL, priv);
  priv->panel_buttons[1] = panel_append_toolbar_button (panel, "status-button", NULL, priv);
  priv->panel_buttons[2] = panel_append_toolbar_button (panel, "spaces-button", G_CALLBACK (spaces_button_cb), priv);
  priv->panel_buttons[3] = panel_append_toolbar_button (panel, "internet-button", NULL, priv);
  priv->panel_buttons[4] = panel_append_toolbar_button (panel, "media-button", NULL, priv);
  priv->panel_buttons[5] = panel_append_toolbar_button (panel, "apps-button", G_CALLBACK (launcher_button_cb), priv);
  priv->panel_buttons[6] = panel_append_toolbar_button (panel, "people-button", NULL, priv);
  priv->panel_buttons[7] = panel_append_toolbar_button (panel, "pasteboard-button", NULL, priv);

  priv->panel_time = g_object_new (CLUTTER_TYPE_LABEL, "font-name", "Sans 14", NULL);
  priv->panel_date = g_object_new (CLUTTER_TYPE_LABEL, "font-name", "Sans 10", NULL);
  update_time_date (priv);

  clutter_label_set_color (CLUTTER_LABEL (priv->panel_time), &lbl_clr);
  clutter_container_add_actor (CLUTTER_CONTAINER (panel), priv->panel_time);
  clutter_actor_set_position (priv->panel_time,
                              (192 / 2) - clutter_actor_get_width (priv->panel_time) / 2, 8);
  

  clutter_label_set_color (CLUTTER_LABEL (priv->panel_date), &lbl_clr);
  clutter_container_add_actor (CLUTTER_CONTAINER (panel), priv->panel_date);
  clutter_actor_set_position (priv->panel_date,
                              (192 / 2) - clutter_actor_get_width (priv->panel_date) / 2, 40);


  g_timeout_add_seconds (60, (GSourceFunc) update_time_date, priv);

  priv->launcher = make_launcher (width);
  clutter_actor_set_y (priv->launcher, clutter_actor_get_height (panel));

  clutter_container_add_actor (CLUTTER_CONTAINER (overlay), priv->launcher);
  clutter_actor_hide (priv->launcher);

  return panel;
}

