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
#include "moblin-netbook-switcher.h"
#include "moblin-netbook-ui.h"
#include "moblin-netbook-panel.h"

extern MutterPlugin mutter_plugin;
static inline MutterPlugin *
mutter_get_plugin ()
{
  return &mutter_plugin;
}

/*******************************************************************
 * Workspace switcher, used to switch between existing workspaces.
 */

void
hide_workspace_switcher (guint32 timestamp)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  if (!priv->workspace_switcher)
    return;

  if (priv->panel_out && !priv->panel_back_in_progress)
    hide_panel ();

  clutter_actor_destroy (priv->workspace_switcher);

  disable_stage (plugin, timestamp);

  priv->workspace_switcher = NULL;
}


/*
 * Calback for clicks on a workspace in the switcher (switches to the
 * appropriate ws).
 */
static gboolean
workspace_input_cb (ClutterActor *clone, ClutterEvent *event, gpointer data)
{
  gint           indx   = GPOINTER_TO_INT (data);
  MutterPlugin  *plugin = mutter_get_plugin ();
  MetaScreen    *screen = mutter_plugin_get_screen (plugin);
  MetaWorkspace *workspace;

  workspace = meta_screen_get_workspace_by_index (screen, indx);

  if (!workspace)
    {
      g_warning ("No workspace specified, %s:%d\n", __FILE__, __LINE__);
      return FALSE;
    }

  hide_workspace_switcher (event->any.time);
  meta_workspace_activate (workspace, event->any.time);

  return FALSE;
}

static gboolean
switcher_keyboard_input_cb (ClutterActor *self,
                            ClutterEvent *event,
                            gpointer      data)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  MetaScreen    *screen = mutter_plugin_get_screen (plugin);
  guint          symbol = clutter_key_event_symbol (&event->key);
  gint           indx;

  if (symbol < CLUTTER_0 || symbol > CLUTTER_9)
    return FALSE;

  /*
   * The keyboard shortcuts are 1-based index, and 0 means new workspace.
   */
  indx = symbol - CLUTTER_0 - 1;

  if (indx >= MAX_WORKSPACES)
    indx = MAX_WORKSPACES - 1;

  if (indx < 0)
    {
      g_printf ("Request for new WS currently not implemented in switcher.\n");
    }
  else
    {
      MetaWorkspace *workspace;

      workspace = meta_screen_get_workspace_by_index (screen, indx);

      if (!workspace)
        {
          g_warning ("No workspace specified, %s:%d\n", __FILE__, __LINE__);
          return TRUE;
        }

      hide_workspace_switcher (event->any.time);
      meta_workspace_activate (workspace, event->any.time);
    }

  return TRUE;
}

static gboolean
workspace_switcher_clone_input_cb (ClutterActor *clone,
                                   ClutterEvent *event,
                                   gpointer      data)
{
  MutterWindow  *mw = data;
  MetaWindow    *window;
  MetaWorkspace *workspace;
  MetaWorkspace *active_workspace;
  MetaScreen    *screen;

  window           = mutter_window_get_meta_window (mw);
  screen           = meta_window_get_screen (window);
  workspace        = meta_window_get_workspace (window);
  active_workspace = meta_screen_get_active_workspace (screen);

  if (!active_workspace || (active_workspace == workspace))
    {
      meta_window_activate_with_workspace (window, event->any.time, workspace);
    }
  else
    {
      meta_workspace_activate_with_focus (workspace, window, event->any.time);
    }

  return FALSE;
}

static ClutterActor*
make_workspace_switcher (GCallback  ws_callback)
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;
  MetaScreen    *screen = mutter_plugin_get_screen (plugin);
  gint           ws_count, active_ws, ws_max_windows = 0;
  gint          *n_windows;
  gint           i;
  NbtkWidget    *table;
  GList         *window_list, *l;

  table = nbtk_table_new ();

  ws_count = meta_screen_get_n_workspaces (screen);

  /* loop through all the workspaces, adding a label for each */
  for (i = 0; i < ws_count; i++)
    {
      ClutterActor *ws_label;
      gchar *s;

      s = g_strdup_printf ("%d", i + 1);

      ws_label = make_workspace_label (s);

      clutter_actor_set_reactive (ws_label, TRUE);

      g_signal_connect (ws_label, "button-press-event",
                        ws_callback, GINT_TO_POINTER (i));

      nbtk_table_add_actor (NBTK_TABLE (table), ws_label, 0, i);
    }

  /* iterate through the windows, adding them to the correct workspace */

  n_windows = g_new0 (gint, ws_count);
  window_list = mutter_plugin_get_windows (plugin);
  for (l = window_list; l; l = g_list_next (l))
    {
      MutterWindow      *mw = l->data;
      ClutterActor      *texture, *clone;
      gint                ws_indx;
      MetaCompWindowType  type;

      ws_indx = mutter_window_get_workspace (mw);
      type = mutter_window_get_window_type (mw);
      /*
       * Only show regular windows that are not sticky (getting stacking order
       * right for sticky windows would be really hard, and since they appear
       * on each workspace, they do not help in identifying which workspace
       * it is).
       */
      if (ws_indx < 0                             ||
          mutter_window_is_override_redirect (mw) ||
          type != META_COMP_WINDOW_NORMAL)
        {
          l = l->next;
          continue;
        }

      texture = mutter_window_get_texture (mw);
      clone   = clutter_clone_texture_new (CLUTTER_TEXTURE (texture));

      clutter_actor_set_reactive (clone, TRUE);

      g_object_weak_ref (G_OBJECT (mw), switcher_origin_weak_notify, clone);
      g_object_weak_ref (G_OBJECT (clone), switcher_clone_weak_notify, mw);

      g_signal_connect (clone, "button-press-event",
                        G_CALLBACK (workspace_switcher_clone_input_cb), mw);

      n_windows[ws_indx]++;
      nbtk_table_add_actor (NBTK_TABLE (table), clone,
                            n_windows[ws_indx], ws_indx);
      clutter_container_child_set (CLUTTER_CONTAINER (table), clone,
                                   "keep-aspect-ratio", TRUE, NULL);

      ws_max_windows = MAX (ws_max_windows, n_windows[ws_indx]);
    }

  g_free (n_windows);

  clutter_actor_set_size (CLUTTER_ACTOR (table),
                          WORKSPACE_CELL_WIDTH * ws_count,
                          WORKSPACE_CELL_HEIGHT * (ws_max_windows + 1));

  /* hilight the active workspace */
  active_ws = meta_screen_get_active_workspace_index (screen);
  nbtk_table_set_active_col (NBTK_TABLE (table), active_ws);

  return CLUTTER_ACTOR (table);
}

/*
 * Constructs and shows the workspace switcher actor.
 */
void
show_workspace_switcher (guint32 timestamp)
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;
  ClutterActor  *overlay;
  ClutterActor  *switcher;
  ClutterActor  *background;
  ClutterActor  *label;
  ClutterActor  *grid;
  gint           screen_width, screen_height;
  gint           switcher_width, switcher_height;
  gint           grid_y;
  guint          grid_w, grid_h;
  gint           panel_y;
  guint          panel_height;
  ClutterColor   background_clr = { 0x44, 0x44, 0x44, 0xdd };
  ClutterColor   label_clr = { 0xff, 0xff, 0xff, 0xff };

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  switcher = clutter_group_new ();
  background = clutter_rectangle_new_with_color (&background_clr);

  label = clutter_label_new_full ("Sans 12",
                                  "You can select a workspace:", &label_clr);
  clutter_actor_realize (label);

  grid_y = clutter_actor_get_height (label) + 3;

  grid = make_workspace_switcher (G_CALLBACK (workspace_input_cb));
  clutter_actor_realize (grid);
  clutter_actor_set_position (grid, 0, grid_y);
  clutter_actor_get_size (grid, &grid_w, &grid_h);

  clutter_container_add (CLUTTER_CONTAINER (switcher),
                         background, label, grid, NULL);

  if (priv->workspace_switcher)
    hide_workspace_switcher (timestamp);

  priv->workspace_switcher = switcher;

  overlay = mutter_plugin_get_overlay_group (plugin);
  clutter_container_add_actor (CLUTTER_CONTAINER (overlay), switcher);

  clutter_actor_get_size (switcher, &switcher_width, &switcher_height);
  clutter_actor_set_size (background, switcher_width, switcher_height);

  panel_height = clutter_actor_get_height (priv->panel);
  panel_y      = clutter_actor_get_y (priv->panel);
  clutter_actor_set_position (switcher, 0, panel_height);

  clutter_actor_set_reactive (switcher, TRUE);

  g_signal_connect (switcher, "key-press-event",
                    G_CALLBACK (switcher_keyboard_input_cb), NULL);

  clutter_grab_keyboard (switcher);

  enable_stage (plugin, timestamp);

  clutter_actor_move_anchor_point_from_gravity (switcher,
                                                CLUTTER_GRAVITY_CENTER);
  
  clutter_actor_set_scale (switcher, 0.0, 0.0);

  tidy_bounce_scale (switcher, 200);

}
