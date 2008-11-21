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
    disable_stage (plugin);
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

/*
 * Panel buttons.
 */
/*
 * Helper method to spawn and application. Will eventually be replaced
 * by libgnome-menu, or something like that.
 */
static void
spawn_app (const gchar *path)
{
  MutterPlugin      *plugin    = mutter_get_plugin ();
  PluginPrivate     *priv      = plugin->plugin_private;
  MetaScreen        *screen    = mutter_plugin_get_screen (plugin);
  MetaDisplay       *display   = meta_screen_get_display (screen);
  guint32            timestamp = meta_display_get_current_time (display);

  Display           *xdpy = mutter_plugin_get_xdisplay (plugin);
  SnLauncherContext *context = NULL;
  const gchar       *id;
  gchar             *argv[2] = {NULL, NULL};

  argv[0] = g_strdup (path);

  if (!path)
    return;

  context = sn_launcher_context_new (priv->sn_display, DefaultScreen (xdpy));

  /* FIXME */
  sn_launcher_context_set_name (context, path);
  sn_launcher_context_set_description (context, path);
  sn_launcher_context_set_binary_name (context, path);

  sn_launcher_context_initiate (context,
                                "mutter-netbook-shell",
                                path, /* bin_name */
				timestamp);

  id = sn_launcher_context_get_startup_id (context);

  if (!g_spawn_async (NULL,
                      &argv[0],
                      NULL,
                      G_SPAWN_SEARCH_PATH,
                      (GSpawnChildSetupFunc)
                      sn_launcher_context_setup_child_process,
                      (gpointer)context,
                      NULL,
                      NULL))
    {
      g_warning ("Failed to launch [%s]", path);
    }

  sn_launcher_context_unref (context);
}

static gboolean
app_launcher_input_cb (ClutterActor *actor,
                       ClutterEvent *event,
                       gpointer      data)
{
  spawn_app (data);

  return FALSE;
}

static ClutterActor *
make_app_launcher (const gchar *name, const gchar *path)
{
  ClutterColor  bkg_clr = {0, 0, 0, 0xff};
  ClutterColor  fg_clr  = {0xff, 0xff, 0xff, 0xff};
  ClutterActor *group = clutter_group_new ();
  ClutterActor *label = clutter_label_new_full ("Sans 12 Bold", name, &fg_clr);
  ClutterActor *bkg = clutter_rectangle_new_with_color (&bkg_clr);
  guint         l_width;

  clutter_actor_realize (label);
  l_width = clutter_actor_get_width (label);
  l_width += 10;

  clutter_actor_set_anchor_point_from_gravity (label, CLUTTER_GRAVITY_CENTER);
  clutter_actor_set_position (label, l_width/2, PANEL_HEIGHT / 2);

  clutter_actor_set_size (bkg, l_width + 5, PANEL_HEIGHT);

  clutter_container_add (CLUTTER_CONTAINER (group), bkg, label, NULL);

  g_signal_connect (group,
                    "button-press-event", G_CALLBACK (app_launcher_input_cb),
                    (gpointer)path);

  clutter_actor_set_reactive (group, TRUE);

  return group;
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
make_workspace_switcher_button ()
{
  ClutterColor  bkg_clr = {0, 0, 0, 0xff};
  ClutterColor  fg_clr  = {0xff, 0xff, 0xff, 0xff};
  ClutterActor *group   = clutter_group_new ();
  ClutterActor *label   = clutter_label_new_full ("Sans 12 Bold",
                                                  "Spaces", &fg_clr);
  ClutterActor *bkg     = clutter_rectangle_new_with_color (&bkg_clr);
  guint         l_width;

  clutter_actor_realize (label);
  l_width = clutter_actor_get_width (label);
  l_width += 10;

  clutter_actor_set_anchor_point_from_gravity (label, CLUTTER_GRAVITY_CENTER);
  clutter_actor_set_position (label, l_width/2, PANEL_HEIGHT / 2);

  clutter_actor_set_size (bkg, l_width + 5, PANEL_HEIGHT);

  clutter_container_add (CLUTTER_CONTAINER (group), bkg, label, NULL);

  g_signal_connect (group,
                    "button-press-event",
                    G_CALLBACK (workspace_button_input_cb),
                    NULL);

  clutter_actor_set_reactive (group, TRUE);

  return group;
}

ClutterActor *
make_panel (gint width)
{
  ClutterActor *panel;
  ClutterActor *background;
  ClutterColor  clr = {0x44, 0x44, 0x44, 0x7f};
  ClutterActor *launcher;
  gint          x, w;

  panel = clutter_group_new ();

  /* FIME -- size and color */
  background = clutter_rectangle_new_with_color (&clr);
  clutter_container_add_actor (CLUTTER_CONTAINER (panel), background);
  clutter_actor_set_size (background, width, PANEL_HEIGHT);

  launcher = make_workspace_switcher_button ();
  clutter_container_add_actor (CLUTTER_CONTAINER (panel), launcher);

  x = clutter_actor_get_x (launcher);
  w = clutter_actor_get_width (launcher);

  launcher = make_app_launcher ("Calculator", "/usr/bin/gcalctool");
  clutter_actor_set_position (launcher, w + x + 10, 0);
  clutter_container_add_actor (CLUTTER_CONTAINER (panel), launcher);

  x = clutter_actor_get_x (launcher);
  w = clutter_actor_get_width (launcher);

  launcher = make_app_launcher ("Editor", "/usr/bin/gedit");
  clutter_actor_set_position (launcher, w + x + 10, 0);
  clutter_container_add_actor (CLUTTER_CONTAINER (panel), launcher);

  return panel;
}

