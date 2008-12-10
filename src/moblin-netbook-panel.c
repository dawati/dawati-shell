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

#include "nutter/nutter-grid.h"

#define BUTTON_WIDTH 66
#define BUTTON_HEIGHT 55
#define BUTTON_SPACING 10

#define TRAY_PADDING   3
#define TRAY_WIDTH   100
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
    {
      guint32         timestamp;
      MetaScreen     *screen = mutter_plugin_get_screen (plugin);
      MetaDisplay    *display = meta_screen_get_display (screen);

      timestamp = meta_display_get_current_time_roundtrip (display);

      disable_stage (plugin, timestamp);
    }
}

static void
on_panel_out_effect_complete (ClutterActor *panel, gpointer data)
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;

  priv->panel_out_in_progress = FALSE;

  enable_stage (plugin, CurrentTime);
}

void
show_panel ()
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;
  gint          x = clutter_actor_get_x (priv->panel);

  priv->panel_out_in_progress  = TRUE;
  clutter_actor_show_all (priv->panel);
  clutter_effect_move (priv->panel_slide_effect,
                       priv->panel, x, 0,
                       on_panel_out_effect_complete,
                       NULL);

  priv->panel_out = TRUE;
}

void
hide_panel ()
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;
  gint           x      = clutter_actor_get_x (priv->panel);

  priv->panel_back_in_progress  = TRUE;

  clutter_effect_move (priv->panel_slide_effect,
                       priv->panel, x, -PANEL_HEIGHT,
                       on_panel_back_effect_complete,
                       NULL);

  /* make sure no buttons are 'active' */
  toggle_buttons_cb (NULL, priv);

  priv->panel_out = FALSE;
}

static gboolean
spaces_button_cb (ClutterActor *actor,
                  GParamSpec   *pspec,
                  gpointer      data)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;
  gboolean       active;

  active = nbtk_button_get_active (NBTK_BUTTON (actor));

  if (!active)
    hide_workspace_switcher ();
  else
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
    g_signal_connect (button, "notify::active", callback, data);

  n_buttons++;
  return CLUTTER_ACTOR (button);
}

static gboolean
launcher_button_cb (ClutterActor *actor,
                    GParamSpec   *pspec,
                    gpointer      data)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;
  gboolean       active;

  active = nbtk_button_get_active (NBTK_BUTTON (actor));

  if (!active)
    {
      clutter_actor_hide (priv->launcher);
    }
  else
    {
      clutter_actor_lower_bottom (priv->launcher);
      clutter_actor_set_position (priv->launcher, 
                                  4, 
                                  -clutter_actor_get_height(priv->launcher)); 

      clutter_actor_show (priv->launcher);
      clutter_effect_move (priv->panel_slide_effect,
                           priv->launcher,
                           4,
                           PANEL_HEIGHT,
                           NULL, NULL);
    }

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

static void
shell_tray_manager_icon_added (ShellTrayManager *mgr,
                               ClutterActor     *icon,
                               ClutterActor     *tray)
{
  clutter_container_add_actor (CLUTTER_CONTAINER (tray), icon);
}

static void
shell_tray_manager_icon_removed (ShellTrayManager *mgr,
                                 ClutterActor     *icon,
                                 ClutterActor     *tray)
{
  clutter_actor_destroy (icon);
}

ClutterActor *
make_panel (gint width)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;
  ClutterActor  *panel;
  ClutterActor  *background, *bg_texture;
  ClutterColor   clr = {0, 0, 0, 0};
  ClutterColor   lbl_clr = {0xc0, 0xc0, 0xc0, 0xff};
  ClutterActor  *launcher, *overlay;
  ClutterActor  *tray;
  gint           x, w;
  GError        *err = NULL;

  overlay = mutter_plugin_get_overlay_group (plugin);

  panel = clutter_group_new ();

  /* FIME -- size and color */
  bg_texture = clutter_texture_new_from_file (PLUGIN_PKGDATADIR "/theme/panel/panel-background.png", &err);
  if (err)
    {
      g_warning (err->message);
      g_clear_error (&err);
    }
  else
    {
      background = nbtk_texture_frame_new (CLUTTER_TEXTURE (bg_texture),
                                           200, 0, 200, 0);
      clutter_container_add_actor (CLUTTER_CONTAINER (panel), background);
      clutter_actor_set_size (background, width, 101);
    }

  priv->panel_buttons[0] = panel_append_toolbar_button (panel, "m-space-button", NULL, priv);
  priv->panel_buttons[1] = panel_append_toolbar_button (panel, "status-button", NULL, priv);
  priv->panel_buttons[2] = panel_append_toolbar_button (panel, "spaces-button", G_CALLBACK (spaces_button_cb), priv);
  priv->panel_buttons[3] = panel_append_toolbar_button (panel, "internet-button", NULL, priv);
  priv->panel_buttons[4] = panel_append_toolbar_button (panel, "media-button", NULL, priv);
  priv->panel_buttons[5] = panel_append_toolbar_button (panel, "apps-button", G_CALLBACK (launcher_button_cb), priv);
  priv->panel_buttons[6] = panel_append_toolbar_button (panel, "people-button", NULL, priv);
  priv->panel_buttons[7] = panel_append_toolbar_button (panel, "pasteboard-button", NULL, priv);

  priv->panel_time = g_object_new (CLUTTER_TYPE_LABEL, "font-name", "Sans 19px", NULL);
  priv->panel_date = g_object_new (CLUTTER_TYPE_LABEL, "font-name", "Sans 11px", NULL);
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

  priv->launcher = make_launcher (width - PANEL_X_PADDING * 2);
  clutter_actor_set_position (priv->launcher, PANEL_X_PADDING, PANEL_HEIGHT);

  clutter_container_add_actor (CLUTTER_CONTAINER (overlay), priv->launcher);
  clutter_actor_hide (priv->launcher);

  priv->tray_manager = g_object_new (SHELL_TYPE_TRAY_MANAGER,
                                     "bg-color", &clr, NULL);

  priv->tray = tray = nutter_grid_new ();

  nutter_grid_set_column_gap (NUTTER_GRID (tray),
                              CLUTTER_UNITS_FROM_DEVICE (TRAY_PADDING));
  nutter_grid_set_max_size (NUTTER_GRID (tray), TRAY_WIDTH, PANEL_HEIGHT);
  clutter_actor_set_size (tray, TRAY_WIDTH, PANEL_HEIGHT);
  nutter_grid_set_valign (NUTTER_GRID (tray), 0.5);
  clutter_actor_set_anchor_point (tray, TRAY_WIDTH, PANEL_HEIGHT/2);
  clutter_actor_set_position (tray, width, PANEL_HEIGHT/2);

  clutter_container_add_actor (CLUTTER_CONTAINER (panel), tray);

  g_signal_connect (priv->tray_manager, "tray-icon-added",
                    G_CALLBACK (shell_tray_manager_icon_added), tray);

  g_signal_connect (priv->tray_manager, "tray-icon-removed",
                    G_CALLBACK (shell_tray_manager_icon_removed), tray);

  shell_tray_manager_manage_stage (priv->tray_manager,
                                   CLUTTER_STAGE (
                                           mutter_plugin_get_stage (plugin)));

  return panel;
}

