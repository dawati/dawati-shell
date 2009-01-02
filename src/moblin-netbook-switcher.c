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
hide_workspace_switcher ()
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  if (!priv->workspace_switcher)
    return;

  clutter_actor_destroy (priv->workspace_switcher);

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

  hide_workspace_switcher ();
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

      hide_workspace_switcher ();
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
make_contents (GCallback  ws_callback)
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;
  MetaScreen    *screen = mutter_plugin_get_screen (plugin);
  gint           ws_count, active_ws, ws_max_windows = 0;
  gint          *n_windows;
  gint           i, screen_width;
  NbtkWidget    *table;
  GList         *window_list, *l;
  NbtkWidget  **spaces;
  NbtkPadding    padding = {CLUTTER_UNITS_FROM_INT (6),
                            CLUTTER_UNITS_FROM_INT (6),
                            CLUTTER_UNITS_FROM_INT (6),
                            CLUTTER_UNITS_FROM_INT (6)};

  table = nbtk_table_new ();
  nbtk_table_set_row_spacing (NBTK_TABLE (table), 4);
  nbtk_table_set_col_spacing (NBTK_TABLE (table), 7);
  nbtk_widget_set_padding (table, &padding);

  clutter_actor_set_name (CLUTTER_ACTOR (table), "switcher-table");

  ws_count = meta_screen_get_n_workspaces (screen);
  active_ws = meta_screen_get_active_workspace_index (screen);

  mutter_plugin_query_screen_size (plugin, &screen_width, &i);

  /* loop through all the workspaces, adding a label for each */
  for (i = 0; i < ws_count; i++)
    {
      NbtkWidget *ws_label;
      gchar *s;

      s = g_strdup_printf ("%d", i + 1);

      ws_label = nbtk_label_new (s);

      if (i == active_ws)
        clutter_actor_set_name (CLUTTER_ACTOR (ws_label), "workspace-title-active");
      nbtk_widget_set_style_class_name (ws_label, "workspace-title");

      clutter_actor_set_reactive (CLUTTER_ACTOR (ws_label), TRUE);

      g_signal_connect (ws_label, "button-press-event",
                        ws_callback, GINT_TO_POINTER (i));

      nbtk_table_add_widget (NBTK_TABLE (table), ws_label, 0, i);
    }

  /* iterate through the windows, adding them to the correct workspace */

  n_windows = g_new0 (gint, ws_count);
  spaces = g_new0 (NbtkWidget*, ws_count);
  window_list = mutter_plugin_get_windows (plugin);
  for (l = window_list; l; l = g_list_next (l))
    {
      MutterWindow       *mw = l->data;
      ClutterActor       *texture, *clone;
      gint                ws_indx;
      MetaCompWindowType  type;
      gint                w, h;

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
          continue;
        }

      /* create the table for this workspace if we don't already have one */
      if (!spaces[ws_indx])
        {
          spaces[ws_indx] = nbtk_table_new ();
          nbtk_table_set_row_spacing (NBTK_TABLE (spaces[ws_indx]), 6);
          nbtk_table_set_col_spacing (NBTK_TABLE (spaces[ws_indx]), 6);
          nbtk_widget_set_padding (spaces[ws_indx], &padding);
          nbtk_widget_set_style_class_name (NBTK_WIDGET (spaces[ws_indx]), "switcher-workspace");
          if (ws_indx == active_ws)
            clutter_actor_set_name (CLUTTER_ACTOR (spaces[ws_indx]), "switcher-workspace-active");
          nbtk_table_add_widget (NBTK_TABLE (table), spaces[ws_indx], 1, ws_indx);
          clutter_container_child_set (CLUTTER_CONTAINER (table), CLUTTER_ACTOR (spaces[ws_indx]),
                                       "y-expand", TRUE, NULL);
        }

      texture = mutter_window_get_texture (mw);
      clone   = clutter_clone_texture_new (CLUTTER_TEXTURE (texture));

      clutter_actor_set_reactive (clone, TRUE);

      g_object_weak_ref (G_OBJECT (mw), switcher_origin_weak_notify, clone);
      g_object_weak_ref (G_OBJECT (clone), switcher_clone_weak_notify, mw);

      g_signal_connect (clone, "button-press-event",
                        G_CALLBACK (workspace_switcher_clone_input_cb), mw);

      n_windows[ws_indx]++;
      nbtk_table_add_actor (NBTK_TABLE (spaces[ws_indx]), clone,
                            n_windows[ws_indx], 0);
      clutter_container_child_set (CLUTTER_CONTAINER (spaces[ws_indx]), clone,
                                   "keep-aspect-ratio", TRUE, NULL);

      clutter_actor_get_size (clone, &h, &w);
      clutter_actor_set_size (clone, h/(gdouble)w * 80.0, 80);

      ws_max_windows = MAX (ws_max_windows, n_windows[ws_indx]);
    }

  g_free (spaces);
  g_free (n_windows);

  /* TODO: hilight the active workspace */

  return CLUTTER_ACTOR (table);
}

/*
 * Constructs and shows the workspace switcher actor.
 */
ClutterActor *
make_workspace_switcher ()
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;
  ClutterActor  *switcher;
  ClutterActor  *label;
  ClutterActor  *grid;
  NbtkWidget    *texture, *footer, *up_button;
  gint           screen_width, screen_height;
  gint           switcher_width, switcher_height;
  gint           grid_y;
  gint           panel_y;
  ClutterColor   label_clr = { 0xff, 0xff, 0xff, 0xff };
  NbtkPadding    padding = {CLUTTER_UNITS_FROM_INT (4),
                            CLUTTER_UNITS_FROM_INT (4),
                            CLUTTER_UNITS_FROM_INT (4),
                            CLUTTER_UNITS_FROM_INT (4)};

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  switcher = clutter_group_new ();

  grid = make_contents (G_CALLBACK (workspace_input_cb));
  clutter_actor_realize (grid);
  clutter_actor_set_position (grid, 0, 0);
  clutter_actor_set_width (grid, screen_width - PANEL_X_PADDING * 2);

  footer = nbtk_table_new ();
  nbtk_widget_set_padding (footer, &padding);
  nbtk_widget_set_style_class_name (footer, "drop-down-footer");

  up_button = nbtk_button_new ();
  nbtk_widget_set_style_class_name (up_button, "drop-down-up-button");
  nbtk_table_add_actor (NBTK_TABLE (footer), CLUTTER_ACTOR (up_button), 0, 0);
  clutter_actor_set_size (CLUTTER_ACTOR (up_button), 23, 21);
  clutter_container_child_set (CLUTTER_CONTAINER (footer),
                               CLUTTER_ACTOR (up_button),
                               "keep-aspect-ratio", TRUE,
                               "x-align", 1.0,
                               NULL);
  g_signal_connect (up_button, "clicked", G_CALLBACK (hide_workspace_switcher), NULL);

  clutter_container_add (CLUTTER_CONTAINER (switcher),
                         grid, footer, NULL);

  if (priv->workspace_switcher)
    hide_workspace_switcher ();

  priv->workspace_switcher = switcher;

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->panel), switcher);
  clutter_actor_set_position (grid, 0, 0);
  clutter_actor_set_position (CLUTTER_ACTOR (footer), 0, clutter_actor_get_height (grid));
  clutter_actor_set_size (CLUTTER_ACTOR (footer), screen_width - PANEL_X_PADDING * 2, 31);

  panel_y      = clutter_actor_get_y (priv->panel);

  clutter_actor_set_reactive (switcher, TRUE);

  g_signal_connect (switcher, "key-press-event",
                    G_CALLBACK (switcher_keyboard_input_cb), NULL);

  clutter_grab_keyboard (switcher);

  clutter_actor_raise (switcher, priv->panel_shadow);

  return switcher;
}
