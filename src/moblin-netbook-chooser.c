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

#include "moblin-netbook-chooser.h"

extern MutterPlugin mutter_plugin;
static inline MutterPlugin *
mutter_get_plugin ()
{
  return &mutter_plugin;
}

/******************************************************************
 * Workspace chooser
 */

struct ws_grid_cb_data
{
  gchar *sn_id;
  gint   workspace;
};

static void
free_ws_grid_cb_data (struct ws_grid_cb_data *data)
{
  g_free (data->sn_id);
  g_slice_free (struct ws_grid_cb_data, data);
}

/*
 * Creates a grid of workspaces, which contains a header row with
 * workspace symbolic icon, and the workspaces below it.
 */
static ClutterActor *
make_workspace_chooser (GCallback    ws_callback,
                        const gchar *sn_id,
                        gint        *n_workspaces)
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;
  GList         *l;
  NbtkWidget    *table;
  gint           screen_width, screen_height;
  GList         *workspaces = NULL;
  gdouble        ws_scale_x, ws_scale_y;
  gint           ws_count = 0;
  MetaScreen    *screen = mutter_plugin_get_screen (plugin);
  gint           active_ws;
  gint           i;
  ClutterActor  *ws_label;

  active_ws = meta_screen_get_n_workspaces (screen);

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  ws_scale_x = (gdouble) WORKSPACE_CELL_WIDTH  / (gdouble) screen_width;
  ws_scale_y = (gdouble) WORKSPACE_CELL_HEIGHT / (gdouble) screen_height;

  table = nbtk_table_new ();
#if 0
  for (i = 0; i < active_ws; ++i)
    {
      gchar *s = g_strdup_printf ("%d", i + 1);
      struct ws_grid_cb_data * wsg_data =
        g_slice_new (struct ws_grid_cb_data);

      wsg_data->sn_id = g_strdup (sn_id);
      wsg_data->workspace = i;

      ws_label = make_workspace_label (s);

      g_signal_connect_data (ws_label, "button-press-event",
                             ws_callback, wsg_data,
                             (GClosureNotify)free_ws_grid_cb_data, 0);

      clutter_actor_set_reactive (ws_label, TRUE);

      nbtk_table_add_actor (NBTK_TABLE (table), ws_label, 0, i);

      g_free (s);
    }
#endif

  l = mutter_plugin_get_windows (plugin);

  while (l)
    {
      MutterWindow       *mw = l->data;
      ClutterActor       *a  = CLUTTER_ACTOR (mw);
      MetaCompWindowType  type;
      gint                ws_indx;
      ClutterActor       *texture;
      ClutterActor       *clone;
      ClutterActor       *workspace = NULL;
      guint               x, y, w, h;
      gdouble             s_x, s_y, s;
      gboolean            active;

      type = mutter_window_get_window_type (mw);
      ws_indx = mutter_window_get_workspace (mw);

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

      clutter_actor_set_reactive (clone, FALSE);

      g_object_weak_ref (G_OBJECT (mw), switcher_origin_weak_notify, clone);
      g_object_weak_ref (G_OBJECT (clone), switcher_clone_weak_notify, mw);

      workspace = ensure_nth_workspace (&workspaces, ws_indx, active_ws);
      g_assert (workspace);


      /*
       * Clones on un-expanded workspaces are left at their original
       * size and position matching that of the original texture.
       *
       * (The entiery WS container is then scaled.)
       */
      clutter_actor_get_position (CLUTTER_ACTOR (mw), &x, &y);
      clutter_actor_set_position (clone,
                                  x + WORKSPACE_BORDER / 2,
                                  y + WORKSPACE_BORDER / 2);

      clutter_container_add_actor (CLUTTER_CONTAINER (workspace), clone);

      l = l->next;
    }

  l = workspaces;
  while (l)
    {
      ClutterActor  *ws = l->data;
      struct ws_grid_cb_data * wsg_data =
        g_slice_new (struct ws_grid_cb_data);

      wsg_data->sn_id = g_strdup (sn_id);
      wsg_data->workspace = ws_count;

      /*
       * Scale unexpanded workspaces to fit the predefined size of the grid cell
       */
      clutter_actor_set_scale (ws, ws_scale_x, ws_scale_y);

      g_signal_connect_data (ws, "button-press-event",
                             ws_callback, wsg_data,
                             (GClosureNotify)free_ws_grid_cb_data, 0);

      clutter_actor_set_reactive (ws, TRUE);

      nbtk_table_add_actor (NBTK_TABLE (table), ws, 1, ws_count);

      ++ws_count;
      l = l->next;
    }

  clutter_actor_set_size (CLUTTER_ACTOR (table),
                          active_ws * WORKSPACE_CELL_WIDTH,
                          2 * WORKSPACE_CELL_HEIGHT);

  if (n_workspaces)
    *n_workspaces = active_ws;

  return CLUTTER_ACTOR (table);
}

static void
finalize_app_startup (const char * sn_id, gint workspace)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;
  gpointer       key, value;

  if (workspace >= MAX_WORKSPACES)
    workspace = MAX_WORKSPACES - 1;

  if (g_hash_table_lookup_extended (priv->sn_hash, sn_id, &key, &value))
    {
      MutterWindow *mcw = NULL;
      SnHashData   *sn_data = value;

      /*
       * Update the workspace index.
       */
      sn_data->workspace = workspace;

      if (sn_data->mcw)
        {
          plugin->map (sn_data->mcw);
        }
    }
}

/*
 * Handles button press on a workspace withing the chooser by placing the
 * starting application on the given workspace.
 */
static gboolean
workspace_chooser_input_cb (ClutterActor *clone,
                            ClutterEvent *event,
                            gpointer      data)
{
  struct ws_grid_cb_data * wsg_data = data;
  gint           indx   = wsg_data->workspace;
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;
  MetaScreen    *screen = mutter_plugin_get_screen (plugin);
  MetaWorkspace *workspace;
  gpointer       key, value;
  const char    *sn_id = wsg_data->sn_id;

  workspace = meta_screen_get_workspace_by_index (screen, indx);

  if (!workspace)
    {
      g_warning ("No workspace specified, %s:%d\n", __FILE__, __LINE__);
      return FALSE;
    }

  hide_workspace_chooser ();

  finalize_app_startup (sn_id, wsg_data->workspace);

  return FALSE;
}

/*
 * Handles click on the New Workspace area in the chooser by placing the
 * starting application on a new workspace.
 */
static gboolean
new_workspace_input_cb (ClutterActor *clone,
                        ClutterEvent *event,
                        gpointer      data)
{
  MutterPlugin  *plugin    = mutter_get_plugin ();
  PluginPrivate *priv      = plugin->plugin_private;
  MetaScreen    *screen    = mutter_plugin_get_screen (plugin);
  struct ws_grid_cb_data * wsg_data = data;
  const char    *sn_id     = wsg_data->sn_id;

  hide_workspace_chooser ();

  meta_screen_append_new_workspace (screen, FALSE, event->any.time);

  finalize_app_startup (sn_id, wsg_data->workspace);

  return FALSE;
}

/*
 * Low-lights and un-low-lights the entire screen below the plugin UI elements.
 */
static void
set_lowlight (gboolean on)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  if (on)
    clutter_actor_show (priv->lowlight);
  else
    clutter_actor_hide (priv->lowlight);
}

/*
 * Creates and shows the workspace chooser.
 */
void
show_workspace_chooser (const gchar * sn_id)
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;
  ClutterActor  *overlay;
  ClutterActor  *switcher;
  ClutterActor  *background;
  ClutterActor  *grid;
  ClutterActor  *label;
  gint           screen_width, screen_height;
  guint          switcher_width, switcher_height;
  guint          grid_width, grid_height;
  guint          label_height;
  ClutterColor   background_clr = { 0, 0, 0, 0xaf };
  ClutterColor   label_clr = { 0xff, 0xff, 0xff, 0xff };
  gint           ws_count = 0;
  ClutterActor  *new_ws;
  ClutterActor  *new_ws_background;
  ClutterActor  *new_ws_label;
  ClutterColor   new_ws_clr = { 0xfd, 0xd9, 0x09, 0x7f};
  ClutterColor   new_ws_text_clr = { 0, 0, 0, 0xff };
  struct ws_grid_cb_data * wsg_data =
        g_slice_new (struct ws_grid_cb_data);

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  switcher = clutter_group_new ();
  background = clutter_rectangle_new_with_color (&background_clr);
  clutter_rectangle_set_border_width (CLUTTER_RECTANGLE (background),
                                      WORKSPACE_CHOOSER_BORDER_WIDTH);
  clutter_rectangle_set_border_color (CLUTTER_RECTANGLE (background),
                                      &label_clr);
  clutter_actor_set_position (background,
                              -2*WORKSPACE_CHOOSER_BORDER_WIDTH,
                              -2*WORKSPACE_CHOOSER_BORDER_WIDTH);

  label = clutter_label_new_full ("Sans 12",
                                  "You can select a workspace:", &label_clr);
  clutter_actor_realize (label);
  label_height = clutter_actor_get_height (label) + 3;

  grid = make_workspace_chooser (G_CALLBACK (workspace_chooser_input_cb),
                                 sn_id, &ws_count);
  clutter_actor_set_position (CLUTTER_ACTOR (grid), 0, label_height);
  clutter_actor_realize (grid);
  clutter_actor_get_size (grid, &grid_width, &grid_height);

  new_ws = clutter_group_new ();
  clutter_actor_set_position (new_ws, grid_width + 5, label_height);

  new_ws_background = clutter_rectangle_new_with_color (&new_ws_clr);
  clutter_actor_set_size (new_ws_background, WORKSPACE_CELL_WIDTH, grid_height);

  new_ws_label = clutter_label_new_full ("Sans 10", "New Workspace",
                                         &new_ws_text_clr);
  clutter_actor_realize (new_ws_label);

  /*
   * Tried to use anchor point in the middle of the label here, but it would
   * appear that the group does not take anchor point into account when
   * caluculating it's size, so it ends up wider than it should by the
   * offset.
   */
  clutter_actor_set_position (new_ws_label, 2,
                              (grid_height -
                               clutter_actor_get_height (new_ws_label))/2);

  clutter_container_add (CLUTTER_CONTAINER (new_ws),
                         new_ws_background, new_ws_label, NULL);

  wsg_data->sn_id     = g_strdup (sn_id);
  wsg_data->workspace = ws_count;

  g_signal_connect_data (new_ws, "button-press-event",
                         G_CALLBACK (new_workspace_input_cb),
                         wsg_data,
                         (GClosureNotify)free_ws_grid_cb_data, 0);

  clutter_actor_set_reactive (new_ws, TRUE);

  clutter_container_add (CLUTTER_CONTAINER (switcher),
                         background, label, grid, new_ws, NULL);

  set_lowlight (TRUE);

  if (priv->workspace_chooser)
    hide_workspace_chooser ();

  priv->workspace_chooser = switcher;

  overlay = mutter_plugin_get_overlay_group (plugin);

  clutter_container_add_actor (CLUTTER_CONTAINER (overlay), switcher);

  clutter_actor_get_size (switcher, &switcher_width, &switcher_height);
  clutter_actor_set_size (background,
                          switcher_width  + 4 * WORKSPACE_CHOOSER_BORDER_WIDTH,
                          switcher_height + 4 * WORKSPACE_CHOOSER_BORDER_WIDTH);

  clutter_actor_set_anchor_point (switcher,
                                  switcher_width/2, switcher_height/2);

  clutter_actor_set_position (switcher, screen_width/2, screen_height/2);

  mutter_plugin_set_stage_reactive (plugin, TRUE);
}

void
hide_workspace_chooser (void)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  if (!priv->workspace_chooser)
    return;

  if (priv->workspace_chooser_timeout)
    {
      g_source_remove (priv->workspace_chooser_timeout);
      priv->workspace_chooser_timeout = 0;
    }

  set_lowlight (FALSE);

  hide_panel ();

  clutter_actor_destroy (priv->workspace_chooser);

  disable_stage (plugin);

  priv->workspace_chooser = NULL;
}

/*
 * Triggers when the user does not interact with the chooser; it places the
 * starting application on a new workspace.
 */
struct ws_chooser_timeout_data
{
  gchar * sn_id;
  gint    workspace;
};

static void
free_ws_chooser_timeout_data (struct ws_chooser_timeout_data *data)
{
  g_free (data->sn_id);
  g_slice_free (struct ws_chooser_timeout_data, data);
}

static gboolean
workspace_chooser_timeout_cb (gpointer data)
{
  MutterPlugin  *plugin    = mutter_get_plugin ();
  PluginPrivate *priv      = plugin->plugin_private;
  MetaScreen    *screen    = mutter_plugin_get_screen (plugin);
  MetaDisplay   *display   = meta_screen_get_display (screen);
  guint32        timestamp = meta_display_get_current_time (display);

  struct ws_chooser_timeout_data *wsc_data = data;

  hide_workspace_chooser ();

  meta_screen_append_new_workspace (screen, FALSE, timestamp);

  finalize_app_startup (wsc_data->sn_id, wsc_data->workspace);

  /* One off */
  return FALSE;
}

void
on_sn_monitor_event (SnMonitorEvent *event,
                     void           *user_data)
{
  MutterPlugin      *plugin = mutter_get_plugin ();
  PluginPrivate     *priv   = plugin->plugin_private;
  SnStartupSequence *sequence;
  const char        *seq_id = NULL, *bin_name = NULL;
  gint               workspace_indx = -2;
  gpointer           key, value;

  sequence = sn_monitor_event_get_startup_sequence (event);

  if (sequence == NULL)
    {
      g_warning ("%s() failed, context / sequence is NULL\n", __func__);
      return;
    }

  seq_id   = sn_startup_sequence_get_id (sequence);
  bin_name = sn_startup_sequence_get_binary_name (sequence);

  if (seq_id == NULL || bin_name == NULL)
    {
      g_warning("%s() failed, seq_id or bin_name NULL \n", __func__ );
      return;
    }

  switch (sn_monitor_event_get_type (event))
    {
    case SN_MONITOR_EVENT_INITIATED:
      {
        SnHashData *sn_data = g_slice_new0 (SnHashData);

        sn_data->workspace = -2;
        sn_data->state = SN_MONITOR_EVENT_INITIATED;
        g_hash_table_insert (priv->sn_hash, g_strdup (seq_id), sn_data);
        show_workspace_chooser (seq_id);
      }
      break;
    case SN_MONITOR_EVENT_CHANGED:
      if (g_hash_table_lookup_extended (priv->sn_hash, seq_id, &key, &value))
        {
          SnHashData *sn_data = value;

          sn_data->state = SN_MONITOR_EVENT_CHANGED;
        }
      break;
    case SN_MONITOR_EVENT_COMPLETED:
      if (g_hash_table_lookup_extended (priv->sn_hash, seq_id, &key, &value))
        {
          MetaScreen *screen = mutter_plugin_get_screen (plugin);
          gint        ws_count = meta_screen_get_n_workspaces (screen);
          SnHashData *sn_data = value;
          struct ws_chooser_timeout_data * wsc_data;

          sn_data->state = SN_MONITOR_EVENT_COMPLETED;

          wsc_data = g_slice_new (struct ws_chooser_timeout_data);
          wsc_data->sn_id = g_strdup (seq_id);
          wsc_data->workspace = ws_count;

          priv->workspace_chooser_timeout =
            g_timeout_add_full (G_PRIORITY_DEFAULT,
                                WORKSPACE_CHOOSER_TIMEOUT,
                                workspace_chooser_timeout_cb,
                                wsc_data,
                                (GDestroyNotify)free_ws_chooser_timeout_data);
        }
      break;
    case SN_MONITOR_EVENT_CANCELED:
      g_hash_table_remove (priv->sn_hash, seq_id);
      break;
    }
}
