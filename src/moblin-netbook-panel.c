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

#define TRAY_PADDING   3
#define TRAY_WIDTH   200
#define TRAY_HEIGHT   24

static void toggle_buttons_cb (NbtkButton *button, gpointer data);

/*
 * The slide-out top panel.
 */
static void
on_panel_back_effect_complete (ClutterActor *panel, gpointer data)
{
  MutterPlugin               *plugin = data;
  MoblinNetbookPluginPrivate *priv   = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  priv->panel_back_in_progress = FALSE;

  if (!priv->workspace_chooser && !priv->workspace_switcher)
    {
      disable_stage (plugin, CurrentTime);
    }
}

struct button_data
{
  MutterPlugin *plugin;
  MnbkControl   control;
};

void
hide_panel (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gint                        x = clutter_actor_get_x (priv->panel);
  guint                       h = clutter_actor_get_height (priv->panel_shadow);
  struct button_data button_data;

  priv->panel_back_in_progress  = TRUE;

  clutter_effect_move (priv->panel_slide_effect,
                       priv->panel, x, -h,
                       on_panel_back_effect_complete,
                       plugin);

  /* make sure no buttons are 'active' */
  button_data.plugin = plugin;
  button_data.control = MNBK_CONTROL_UNKNOWN;
  toggle_buttons_cb (NULL, &button_data);

  priv->panel_out = FALSE;
}

static void
toggle_buttons_cb (NbtkButton *button, gpointer data)
{
  struct button_data         *button_data = data;
  MutterPlugin               *plugin = button_data->plugin;
  MoblinNetbookPluginPrivate *priv   = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gint                        i;
  MnbkControl                 control = button_data->control;

  for (i = 0; i < 8; i++)
    if (priv->panel_buttons[i] != (ClutterActor*)button)
      nbtk_button_set_active (NBTK_BUTTON (priv->panel_buttons[i]), FALSE);

  if (control != MNBK_CONTROL_UNKNOWN)
    {
      gboolean active = nbtk_button_get_active (button);

      toggle_control (plugin, control, active);
    }
}

static ClutterActor*
panel_append_toolbar_button (MutterPlugin  *plugin,
                             ClutterActor  *container,
                             gchar         *name,
                             gchar         *tooltip,
                             MnbkControl    control)
{
  static int n_buttons = 0;

  NbtkWidget *button;
  struct button_data *button_data = g_new (struct button_data, 1);

  button_data->control = control;
  button_data->plugin  = plugin;

  button = nbtk_button_new ();
  nbtk_button_set_toggle_mode (NBTK_BUTTON (button), TRUE);
  nbtk_button_set_tooltip (NBTK_BUTTON (button), tooltip);
  clutter_actor_set_name (CLUTTER_ACTOR (button), name);
  clutter_actor_set_size (CLUTTER_ACTOR (button), BUTTON_WIDTH, BUTTON_HEIGHT);
  clutter_actor_set_position (CLUTTER_ACTOR (button),
                              213 + (BUTTON_WIDTH * n_buttons) +
                              (BUTTON_SPACING * n_buttons),
                              PANEL_HEIGHT - BUTTON_HEIGHT);

  clutter_container_add_actor (CLUTTER_CONTAINER (container),
                               CLUTTER_ACTOR (button));

  g_object_set (G_OBJECT (button),
                "transition-type", NBTK_TRANSITION_BOUNCE,
                "transition-duration", 500, NULL);

  g_signal_connect_data (button, "clicked", G_CALLBACK (toggle_buttons_cb),
                         button_data, (GClosureNotify)g_free, 0);

  n_buttons++;
  return CLUTTER_ACTOR (button);
}

static gboolean
update_time_date (MoblinNetbookPluginPrivate *priv)
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
  nbtk_label_set_text (NBTK_LABEL (priv->panel_time), time_str);

  if (tmp)
    strftime (time_str, 64, "%B %e, %Y", tmp);
  else
    snprintf (time_str, 64, "Date");
  nbtk_label_set_text (NBTK_LABEL (priv->panel_date), time_str);

  return TRUE;
}

static void
shell_tray_manager_icon_added (ShellTrayManager *mgr,
                               ClutterActor     *icon,
                               ClutterActor     *tray)
{
  static col = 0;
  nbtk_table_add_actor (NBTK_TABLE (tray), icon, 0, col++);
  clutter_container_child_set (CLUTTER_CONTAINER (tray), icon,
                               "keep-aspect-ratio", TRUE, NULL);
}

static void
shell_tray_manager_icon_removed (ShellTrayManager *mgr,
                                 ClutterActor     *icon,
                                 ClutterActor     *tray)
{
  clutter_actor_destroy (icon);
}

ClutterActor *
make_panel (MutterPlugin *plugin, gint width)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  ClutterActor               *panel;
  ClutterActor               *shadow;
  ClutterActor               *tray;
  ClutterActor               *background, *bg_texture;
  ClutterColor                clr = {0x0, 0x0, 0x0, 0xce};
  ClutterActor               *launcher, *overlay;
  gint                        x, w;
  GError                     *err = NULL;

  overlay = mutter_plugin_get_overlay_group (plugin);

  panel = clutter_group_new ();

  /* FIME -- size and color */
  bg_texture = clutter_texture_new_from_file (PLUGIN_PKGDATADIR
                                              "/theme/panel/panel-shadow.png",
                                              &err);
  if (err)
    {
      g_warning (err->message);
      g_clear_error (&err);
    }
  else
    {
      shadow = nbtk_texture_frame_new (CLUTTER_TEXTURE (bg_texture),
                                       200, 0, 200, 0);
      clutter_actor_set_size (shadow, width, 101);
      clutter_container_add_actor (CLUTTER_CONTAINER (panel), shadow);
      priv->panel_shadow = shadow;
    }

  bg_texture = clutter_texture_new_from_file (PLUGIN_PKGDATADIR
                                            "/theme/panel/panel-background.png",
                                              &err);
  if (err)
    {
      g_warning (err->message);
      g_clear_error (&err);
    }
  else
    {
      background = nbtk_texture_frame_new (CLUTTER_TEXTURE (bg_texture),
                                           200, 0, 200, 0);
      clutter_actor_set_size (background, width - 8, PANEL_HEIGHT);
      clutter_actor_set_x (background, 4);
      clutter_container_add_actor (CLUTTER_CONTAINER (panel), background);
    }

  priv->panel_buttons[0] = panel_append_toolbar_button (plugin,
                                                        panel,
                                                        "m-space-button",
                                                        "m_zone",
                                                        MNBK_CONTROL_MSPACE);

  priv->panel_buttons[1] = panel_append_toolbar_button (plugin,
                                                        panel,
                                                        "status-button",
                                                        "status",
                                                        MNBK_CONTROL_STATUS);

  priv->panel_buttons[2] = panel_append_toolbar_button (plugin,
                                                        panel,
                                                        "spaces-button",
                                                        "spaces",
                                                        MNBK_CONTROL_SPACES);

  priv->panel_buttons[3] = panel_append_toolbar_button (plugin,
                                                        panel,
                                                        "internet-button",
                                                        "internet",
                                                        MNBK_CONTROL_INTERNET);

  priv->panel_buttons[4] = panel_append_toolbar_button (plugin,
                                                        panel,
                                                        "media-button",
                                                        "media",
                                                        MNBK_CONTROL_MEDIA);

  priv->panel_buttons[5] = panel_append_toolbar_button (plugin,
                                                        panel,
                                                        "apps-button",
                                                        "applications",
                                                     MNBK_CONTROL_APPLICATIONS);

  priv->panel_buttons[6] = panel_append_toolbar_button (plugin,
                                                        panel,
                                                        "people-button",
                                                        "people",
                                                        MNBK_CONTROL_PEOPLE);

  priv->panel_buttons[7] = panel_append_toolbar_button (plugin,
                                                        panel,
                                                        "pasteboard-button",
                                                        "pasteboard",
                                                     MNBK_CONTROL_PASTEBOARD);

  priv->panel_time = nbtk_label_new ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->panel_time), "time-label");
  priv->panel_date = nbtk_label_new ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->panel_date), "date-label");
  update_time_date (priv);

  clutter_container_add_actor (CLUTTER_CONTAINER (panel),
                               CLUTTER_ACTOR (priv->panel_time));
  clutter_actor_set_position (CLUTTER_ACTOR (priv->panel_time),
       (192 / 2) - clutter_actor_get_width (CLUTTER_ACTOR (priv->panel_time)) /
                              2, 8);


  clutter_container_add_actor (CLUTTER_CONTAINER (panel), CLUTTER_ACTOR (priv->panel_date));
  clutter_actor_set_position (CLUTTER_ACTOR (priv->panel_date),
       (192 / 2) - clutter_actor_get_width (CLUTTER_ACTOR (priv->panel_date)) /
                              2, 40);


  g_timeout_add_seconds (60, (GSourceFunc) update_time_date, priv);

  priv->launcher = make_launcher (plugin, width - PANEL_X_PADDING * 2);
  clutter_actor_set_position (priv->launcher, PANEL_X_PADDING, PANEL_HEIGHT);

  clutter_container_add_actor (CLUTTER_CONTAINER (panel), priv->launcher);

  clutter_actor_hide (priv->launcher);

  priv->tray_manager = g_object_new (SHELL_TYPE_TRAY_MANAGER,
                                     "bg-color", &clr,
                                     "mutter-plugin", plugin,
                                     NULL);

  priv->tray = tray = CLUTTER_ACTOR (nbtk_table_new ());

  nbtk_table_set_col_spacing (NBTK_TABLE (tray), TRAY_PADDING);
  clutter_actor_set_size (tray, TRAY_WIDTH, TRAY_HEIGHT);
  clutter_actor_set_anchor_point (tray, TRAY_WIDTH, TRAY_HEIGHT/2);
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

