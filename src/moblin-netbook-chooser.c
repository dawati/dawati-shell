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

#include <nbtk/nbtk-texture-frame.h>

#define WORKSPACE_CHOOSER_BORDER_LEFT   2
#define WORKSPACE_CHOOSER_BORDER_RIGHT  3
#define WORKSPACE_CHOOSER_BORDER_TOP    4
#define WORKSPACE_CHOOSER_BORDER_BOTTOM 4
#define WORKSPACE_CHOOSER_BORDER_PAD    8
#define WORKSPACE_CHOOSER_CELL_PAD      4
#define WORKSPACE_CHOOSER_LABEL_HEIGHT  40

/******************************************************************
 * Workspace chooser
 */

typedef struct SnHashData    SnHashData;

struct SnHashData
{
  MutterWindow       *mcw;
  gint                workspace;
  SnMonitorEventType  state;
  gboolean            without_chooser : 1;
};

struct ws_grid_cb_data
{
  gchar *sn_id;
  gint   workspace;
  MutterPlugin *plugin;
};

static void
free_ws_grid_cb_data (struct ws_grid_cb_data *data)
{
  g_free (data->sn_id);
  g_slice_free (struct ws_grid_cb_data, data);
}

static void
move_window_to_workspace (MutterWindow *mcw,
                          gint          workspace_index,
                          guint32       timestamp)
{
  if (workspace_index > -2)
    {
      MetaWindow  *mw      = mutter_window_get_meta_window (mcw);
      MetaScreen  *screen  = meta_window_get_screen (mw);

      if (mw)
        {
          /*
           * Move the window to the requested workspace; if the window is not
           * sticky, activate the workspace as well.
           */
          meta_window_change_workspace_by_index (mw, workspace_index, TRUE,
                                                 timestamp);

          if (workspace_index > -1)
            {
              MetaDisplay   *display = meta_screen_get_display (screen);
              MetaWorkspace *workspace;

              workspace =
                meta_screen_get_workspace_by_index (screen,
                                                    workspace_index);

              if (workspace)
                meta_workspace_activate_with_focus (workspace, mw, timestamp);
            }
        }
    }
}

static void
finalize_app_startup (const char * sn_id, gint workspace, guint32 timestamp,
                      MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gpointer                    key, value;

  if (workspace >= MAX_WORKSPACES)
    workspace = MAX_WORKSPACES - 1;

  if (g_hash_table_lookup_extended (priv->sn_hash, sn_id, &key, &value))
    {
      SnHashData *sn_data = value;

      /*
       * Update the workspace index.
       */
      sn_data->workspace = workspace;

      /*
       * If the window has already mapped (i.e., we have its MutterWindow),
       * we move it to the requested workspace so that the switche workspace
       * effect is triggered.
       *
       * If the WS effect is no longer in progress, we map the window.
       */
      if (sn_data->mcw)
        move_window_to_workspace (sn_data->mcw, workspace, timestamp);

      if (sn_data->mcw && !priv->desktop_switch_in_progress)
        {
          MutterPluginClass *klass = MUTTER_PLUGIN_GET_CLASS (plugin);
          klass->map (plugin, sn_data->mcw);
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
  struct ws_grid_cb_data     *wsg_data = data;
  gint                        indx   = wsg_data->workspace;
  MutterPlugin               *plugin = wsg_data->plugin;
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen = mutter_plugin_get_screen (plugin);
  MetaWorkspace              *workspace;
  gpointer                    key, value;
  const char                 *sn_id = wsg_data->sn_id;
  gint                        active;

  workspace = meta_screen_get_workspace_by_index (screen, indx);
  active    = meta_screen_get_active_workspace_index (screen);

  if (!workspace)
    {
      g_warning ("No workspace specified, %s:%d\n", __FILE__, __LINE__);
      return FALSE;
    }

  if (active != indx)
    priv->desktop_switch_in_progress = TRUE;

  hide_workspace_chooser (plugin, event->any.time);

  finalize_app_startup (sn_id, wsg_data->workspace, event->any.time, plugin);

  return FALSE;
}

struct kbd_data
{
  char         *sn_id;
  MutterPlugin *plugin;
};

static void
kbd_data_free (struct kbd_data *kbd_data)
{
  g_free (kbd_data->sn_id);
  g_free (kbd_data);
}

static gboolean
chooser_keyboard_input_cb (ClutterActor *self,
                           ClutterEvent *event,
                           gpointer      data)
{
  struct kbd_data            *kbd_data = data;
  MutterPlugin               *plugin = kbd_data->plugin;
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen = mutter_plugin_get_screen (plugin);
  const char                 *sn_id = kbd_data->sn_id;
  guint                       symbol = clutter_key_event_symbol (&event->key);
  gint                        indx;
  gint                        active;

  if (symbol < CLUTTER_0 || symbol > CLUTTER_9)
    return FALSE;

  /*
   * The keyboard shortcuts are 1-based index, and 0 means new workspace.
   */
  indx = symbol - CLUTTER_0 - 1;

  if (indx >= MAX_WORKSPACES)
    indx = MAX_WORKSPACES - 1;
  else if (indx == -1)
    indx = -2;

  active = meta_screen_get_active_workspace_index (screen);

  if (active != indx)
    priv->desktop_switch_in_progress = TRUE;

  hide_workspace_chooser (plugin, event->any.time);

  finalize_app_startup (sn_id, indx, event->any.time, plugin);

  return TRUE;
}

/*
 * Creates a simple busy spinner; the returned actor has anchor point set
 * in its center for convenience.
 */
static ClutterActor *
make_spinner (void)
{
  ClutterActor *spinner;
  guint         s_w, s_h;

  static ClutterBehaviour *beh = NULL;

  spinner = clutter_texture_new_from_file (PLUGIN_PKGDATADIR
                                           "/theme/generic/spinner.png", NULL);

  if (!spinner)
    return NULL;

  clutter_actor_realize (spinner);
  clutter_actor_get_size (spinner, &s_w, &s_h);

  clutter_actor_set_anchor_point (spinner, s_w/2, s_h/2);

  if (!beh)
    {
      ClutterAlpha    *alpha;
      ClutterTimeline *timeline;

      timeline = clutter_timeline_new_for_duration (MNBTK_SPINNER_ITERVAL);
      clutter_timeline_set_loop (timeline, TRUE);

      alpha = clutter_alpha_new_full (timeline,
                                      CLUTTER_ALPHA_RAMP_INC,
                                      NULL, NULL);

      beh = clutter_behaviour_rotate_new (alpha, CLUTTER_Z_AXIS,
                                          CLUTTER_ROTATE_CW,
                                          0.0, 360.0);

      clutter_timeline_start (timeline);
    }

  clutter_behaviour_apply (beh, spinner);

  return spinner;
}

/*
 * Creates an iconic representation of the workspace with the label provided.
 */
static ClutterActor *
make_background (const gchar *text, guint width, guint height,
                 gboolean selected, gboolean with_spinner)
{
  ClutterActor *group, *bck, *label_actor;
  ClutterLabel *label;
  ClutterColor  white = { 0xff, 0xff, 0xff, 0xff };
  guint         l_w, l_h;

  group = clutter_group_new ();

  if (selected)
    bck = clutter_texture_new_from_file (PLUGIN_PKGDATADIR
                                         "/theme/chooser/space-selected.png", NULL);
  else
    bck = clutter_texture_new_from_file (PLUGIN_PKGDATADIR
                                         "/theme/chooser/space-unselected.png", NULL);

  if (bck)
    {
      ClutterActor *frame;

      frame = nbtk_texture_frame_new (CLUTTER_TEXTURE (bck), 15, 15, 15, 15);

      clutter_actor_set_size (frame, width, WORKSPACE_CHOOSER_LABEL_HEIGHT);

      g_object_unref (bck);
      clutter_container_add_actor (CLUTTER_CONTAINER (group), frame);
    }


  if (selected)
    bck = clutter_texture_new_from_file (PLUGIN_PKGDATADIR
                                         "/theme/chooser/thumb-selected.png",
                                         NULL);
  else
    bck = clutter_texture_new_from_file (PLUGIN_PKGDATADIR
                                         "/theme/chooser/thumb-unselected.png",
                                         NULL);

  if (bck)
    {
      ClutterActor *frame;
      frame = nbtk_texture_frame_new (CLUTTER_TEXTURE (bck), 15, 15, 15, 15);

      clutter_actor_set_size (frame, width, height);
      clutter_actor_set_y (frame, WORKSPACE_CHOOSER_LABEL_HEIGHT +
                           WORKSPACE_CHOOSER_CELL_PAD);

      g_object_unref (bck);
      clutter_container_add_actor (CLUTTER_CONTAINER (group), frame);
    }

  if (text)
    {
      label_actor = clutter_label_new ();
      label = CLUTTER_LABEL (label_actor);

      clutter_label_set_font_name (label, "Sans 8");
      clutter_label_set_color (label, &white);
      clutter_label_set_text (label, text);

      clutter_actor_realize (label_actor);
      clutter_actor_get_size (label_actor, &l_w, &l_h);

      clutter_actor_set_position (label_actor,
                                  (width - l_w)/2,
                                  (WORKSPACE_CHOOSER_LABEL_HEIGHT - l_h)/2);

      clutter_container_add_actor (CLUTTER_CONTAINER (group), label_actor);
    }

  if (with_spinner)
    {
      ClutterActor *spinner = make_spinner ();
      clutter_actor_set_position (spinner,
                                  width / 2,
                                  height /2 + WORKSPACE_CHOOSER_LABEL_HEIGHT +
                                  WORKSPACE_CHOOSER_CELL_PAD);

      clutter_container_add_actor (CLUTTER_CONTAINER (group), spinner);
    }

  return group;
}


static ClutterActor *
make_nth_workspace (GList **list, gint n, gint active, MutterPlugin *plugin)
{
  GList         *l      = *list;
  GList         *tmp    = NULL;
  gint           i      = 0;
  gint           screen_width, screen_height;

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  while (l)
    {
      if (i == n)
        return l->data;

      l = l->next;
      ++i;
    }

  g_assert (i <= n);

  while (i <= n)
    {
      ClutterActor *group;
      /*
       * For non-expanded group, we use NutterScaleGroup container, which
       * allows us to apply scale to the workspace en mass.
       */
      group = nutter_scale_group_new ();

      tmp = g_list_append (tmp, group);

      ++i;
    }

  g_assert (tmp);

  *list = g_list_concat (*list, tmp);

  return g_list_last (*list)->data;
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
  struct ws_grid_cb_data     *wsg_data = data;
  MutterPlugin               *plugin   = wsg_data->plugin;
  MoblinNetbookPluginPrivate *priv     = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen   = mutter_plugin_get_screen (plugin);
  const char                 *sn_id    = wsg_data->sn_id;

  hide_workspace_chooser (plugin, event->any.time);

  priv->desktop_switch_in_progress = TRUE;

  if (wsg_data->workspace >= MAX_WORKSPACES)
    wsg_data->workspace = MAX_WORKSPACES - 1;
  else
    meta_screen_append_new_workspace (screen, FALSE, event->any.time);

  finalize_app_startup (sn_id, wsg_data->workspace, event->any.time,
                        wsg_data->plugin);

  return FALSE;
}

static void chooser_clone_weak_notify (gpointer data, GObject *object);

static void
chooser_origin_weak_notify (gpointer data, GObject *object)
{
  ClutterActor *clone = data;

  /*
   * The original MutterWindow destroyed; remove the weak reference the
   * we added to the clone referencing the original window, then
   * destroy the clone.
   */
  g_object_weak_unref (G_OBJECT (clone), chooser_clone_weak_notify, object);
  clutter_actor_destroy (clone);
}

static void
chooser_clone_weak_notify (gpointer data, GObject *object)
{
  ClutterActor *origin = data;

  /*
   * Clone destroyed -- this function gets only called whent the clone
   * is destroyed while the original MutterWindow still exists, so remove
   * the weak reference we added on the origin for sake of the clone.
   */
  g_object_weak_unref (G_OBJECT (origin), chooser_origin_weak_notify, object);
}

/*
 * Creates a grid of workspaces.
 */
static ClutterActor *
make_workspace_chooser (const gchar *sn_id, gint *n_workspaces,
                        MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  GList                      *l;
  NbtkWidget                 *table;
  gint                        screen_width, screen_height;
  gint                        cell_width, cell_height;
  GList                      *workspaces = NULL;
  gdouble                     ws_scale_x, ws_scale_y;
  gint                        ws_count = 0;
  MetaScreen                 *screen = mutter_plugin_get_screen (plugin);
  gint                        active_ws, n_ws;
  ClutterActor               *new_ws;
  struct ws_grid_cb_data     *new_wsg_data =
    g_slice_new (struct ws_grid_cb_data);

  active_ws = meta_screen_get_active_workspace_index (screen);
  n_ws = meta_screen_get_n_workspaces (screen);

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  cell_width = (screen_width -
                MAX_WORKSPACES * 2 * WORKSPACE_CHOOSER_CELL_PAD) /
    MAX_WORKSPACES;

  cell_height = screen_height * cell_width / screen_width;

  ws_scale_x = (gdouble) (cell_width -
                          2*WORKSPACE_CHOOSER_CELL_PAD)/ (gdouble)screen_width;
  ws_scale_y = (gdouble) (cell_height -
                          2*WORKSPACE_CHOOSER_CELL_PAD) /(gdouble)screen_height;

  table = nbtk_table_new ();

  nbtk_table_set_col_spacing (NBTK_TABLE (table), WORKSPACE_CHOOSER_BORDER_PAD);

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

      g_object_weak_ref (G_OBJECT (mw), chooser_origin_weak_notify, clone);
      g_object_weak_ref (G_OBJECT (clone), chooser_clone_weak_notify, mw);

      workspace = make_nth_workspace (&workspaces, ws_indx, active_ws, plugin);
      g_assert (workspace);


      /*
       * Clones on un-expanded workspaces are left at their original
       * size and position matching that of the original texture.
       *
       * (The entiery WS container is then scaled.)
       */
      clutter_actor_get_position (CLUTTER_ACTOR (mw), &x, &y);
      clutter_actor_set_position (clone, x, y);

      clutter_container_add_actor (CLUTTER_CONTAINER (workspace), clone);

      l = l->next;
    }

  l = workspaces;
  while (l)
    {
      ClutterActor  *ws = l->data;
      gchar         *s = g_strdup_printf ("space %d", ws_count + 1);
      ClutterActor  *cell;

      struct ws_grid_cb_data * wsg_data =
        g_slice_new (struct ws_grid_cb_data);

      wsg_data->sn_id = g_strdup (sn_id);
      wsg_data->workspace = ws_count;
      wsg_data->plugin = plugin;

      cell = make_background (s,
                              cell_width, cell_height, (ws_count == active_ws),
                              FALSE);

      g_free (s);

      /*
       * We connect to the label, as the workspace size depends on its content
       * so we do not get the signal reliably for the entire cell.
       */
      g_signal_connect_data (cell, "button-press-event",
                             G_CALLBACK (workspace_chooser_input_cb), wsg_data,
                             (GClosureNotify)free_ws_grid_cb_data, 0);

      clutter_actor_set_reactive (cell, TRUE);

      /*
       * Scale unexpanded workspaces to fit the predefined size of the grid cell
       */
      clutter_actor_set_scale (ws, ws_scale_x, ws_scale_y);
      clutter_actor_set_position (ws,
                                  WORKSPACE_CHOOSER_CELL_PAD,
                                  2 * WORKSPACE_CHOOSER_CELL_PAD +
                                  WORKSPACE_CHOOSER_LABEL_HEIGHT);

      clutter_container_add_actor (CLUTTER_CONTAINER (cell), ws);

      nbtk_table_add_actor (NBTK_TABLE (table), cell, 0, ws_count);

      ++ws_count;
      l = l->next;
    }

  new_ws = make_background ("new space (0)", cell_width, cell_height,
                            FALSE, TRUE);

  new_wsg_data->sn_id     = g_strdup (sn_id);
  new_wsg_data->workspace = ws_count;
  new_wsg_data->plugin    = plugin;

  g_signal_connect_data (new_ws, "button-press-event",
                         G_CALLBACK (new_workspace_input_cb),
                         new_wsg_data,
                         (GClosureNotify)free_ws_grid_cb_data, 0);

  clutter_actor_set_reactive (new_ws, TRUE);

  nbtk_table_add_actor (NBTK_TABLE (table), new_ws, 0, ws_count);

  if (n_workspaces)
    *n_workspaces = n_ws;

  /*
   * FIXME -- this is a workaround for clutter bug 1338; remove once fixed.
   */
  clutter_actor_set_size (CLUTTER_ACTOR (table),
                          clutter_actor_get_width (CLUTTER_ACTOR (table)),
                          cell_height + WORKSPACE_CHOOSER_CELL_PAD +
                          WORKSPACE_CHOOSER_LABEL_HEIGHT);

  return CLUTTER_ACTOR (table);
}

/*
 * Low-lights and un-low-lights the entire screen below the plugin UI elements.
 */
static void
set_lowlight (MutterPlugin *plugin, gboolean on)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  if (on)
    clutter_actor_show (priv->lowlight);
  else
    clutter_actor_hide (priv->lowlight);
}

/*
 * Creates and shows the workspace chooser.
 */
void
show_workspace_chooser (MutterPlugin *plugin,
                        const gchar * sn_id, guint32 timestamp)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  ClutterActor               *overlay;
  ClutterActor               *switcher;
  ClutterActor               *bck, *frame;
  ClutterActor               *grid;
  ClutterActor               *label;
  gint                        screen_width, screen_height;
  guint                       switcher_width, switcher_height;
  guint                       label_height;
  ClutterColor                label_clr = { 0xff, 0xff, 0xff, 0xff };
  gint                        ws_count = 0;
  struct kbd_data            *kbd_data;

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  switcher = clutter_group_new ();
  bck =clutter_texture_new_from_file (PLUGIN_PKGDATADIR
                                      "/theme/chooser/background.png", NULL);

  frame = nbtk_texture_frame_new (CLUTTER_TEXTURE (bck), 15, 15, 15, 15);

  /*
   * Release the original pixmap, so we do not leak.
   * TODO -- check this is legal.
   */
  g_object_unref (bck);

  clutter_actor_set_position (frame,
                              -(WORKSPACE_CHOOSER_BORDER_LEFT +
                                WORKSPACE_CHOOSER_BORDER_PAD),
                              -(WORKSPACE_CHOOSER_BORDER_TOP +
                                WORKSPACE_CHOOSER_BORDER_PAD));

  /*
   * Set initial size to 0 so we can measure the size of the switcher without
   * the background.
   */
  clutter_actor_set_size (frame, 0, 0);

  label = clutter_label_new_full ("Sans 9",
                                  "Choose space for application:", &label_clr);
  clutter_actor_realize (label);
  label_height = clutter_actor_get_height (label) + 3;

  grid = make_workspace_chooser (sn_id, &ws_count, plugin);
  clutter_actor_set_position (CLUTTER_ACTOR (grid), 0, label_height);

  clutter_container_add (CLUTTER_CONTAINER (switcher),
                         frame, label, grid, NULL);

  set_lowlight (plugin, TRUE);

  if (priv->workspace_chooser)
    hide_workspace_chooser (plugin, timestamp);

  priv->workspace_chooser = switcher;

  overlay = mutter_plugin_get_overlay_group (plugin);

  clutter_container_add_actor (CLUTTER_CONTAINER (overlay), switcher);

  clutter_actor_realize (switcher);
  clutter_actor_get_size (switcher, &switcher_width, &switcher_height);

  clutter_actor_set_size (frame,
                          switcher_width  + WORKSPACE_CHOOSER_BORDER_LEFT +
                          WORKSPACE_CHOOSER_BORDER_RIGHT +
                          2 * WORKSPACE_CHOOSER_BORDER_PAD,
                          switcher_height + WORKSPACE_CHOOSER_BORDER_TOP +
                          WORKSPACE_CHOOSER_BORDER_BOTTOM +
                          2 * WORKSPACE_CHOOSER_BORDER_PAD);

  clutter_actor_set_anchor_point (switcher,
                                  switcher_width/2, switcher_height/2);

  clutter_actor_set_position (switcher, screen_width/2, screen_height/2);

  clutter_actor_set_reactive (switcher, TRUE);

  kbd_data = g_new (struct kbd_data, 1);
  kbd_data->sn_id = g_strdup (sn_id);
  kbd_data->plugin = plugin;

  g_signal_connect_data (switcher, "key-press-event",
                         G_CALLBACK (chooser_keyboard_input_cb),
                         kbd_data, (GClosureNotify) kbd_data_free, 0);

  clutter_grab_keyboard (switcher);

  enable_stage (plugin, timestamp);
}

void
hide_workspace_chooser (MutterPlugin *plugin, guint32 timestamp)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  if (!priv->workspace_chooser)
    return;

  if (priv->workspace_chooser_timeout)
    {
      g_source_remove (priv->workspace_chooser_timeout);
      priv->workspace_chooser_timeout = 0;
    }

  set_lowlight (plugin, FALSE);

  hide_panel (plugin);

  clutter_actor_destroy (priv->workspace_chooser);

  disable_stage (plugin, timestamp);

  priv->workspace_chooser = NULL;
}

/*
 * Triggers when the user does not interact with the chooser; it places the
 * starting application on a new workspace.
 */
struct ws_chooser_timeout_data
{
  gchar        *sn_id;
  gint          workspace;
  MutterPlugin *plugin;
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
  struct ws_chooser_timeout_data *wsc_data = data;
  MutterPlugin                   *plugin   = wsc_data->plugin;
  MoblinNetbookPluginPrivate     *priv = MOBLIN_NETBOOK_PLUGIN(plugin)->priv;
  MetaScreen                     *screen   = mutter_plugin_get_screen (plugin);
  MetaDisplay                    *display  = meta_screen_get_display (screen);
  guint32                         timestamp;

  timestamp = meta_display_get_current_time_roundtrip (display);

  if (!priv->workspace_chooser_timeout)
    {
      g_message ("Workspace timeout triggered after user input, ignoring\n");
      return FALSE;
    }

  priv->desktop_switch_in_progress = TRUE;

  hide_workspace_chooser (plugin, timestamp);

  if (wsc_data->workspace >= MAX_WORKSPACES)
    wsc_data->workspace = MAX_WORKSPACES - 1;
  else
    meta_screen_append_new_workspace (screen, FALSE, timestamp);

  finalize_app_startup (wsc_data->sn_id, wsc_data->workspace, timestamp,
                        plugin);

  /* One off */
  return FALSE;
}

static void
on_sn_monitor_event (SnMonitorEvent *event, gpointer data)
{
  MutterPlugin               *plugin = data;
  MoblinNetbookPluginPrivate *priv   = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  SnStartupSequence          *sequence;
  const char                 *seq_id = NULL, *bin_name = NULL;
  gint                        workspace_indx = -2;
  gpointer                    key, value;

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
        SnHashData *sn_data;
        guint32     timestamp = sn_startup_sequence_get_timestamp (sequence);

        if (g_hash_table_lookup_extended (priv->sn_hash, seq_id, &key, &value))
          {
            sn_data = value;
          }
        else
          {
            sn_data = g_slice_new0 (SnHashData);

            sn_data->workspace = -2;

            g_hash_table_insert (priv->sn_hash, g_strdup (seq_id), sn_data);
          }

          sn_data->state = SN_MONITOR_EVENT_INITIATED;

          if (!sn_data->without_chooser)
            show_workspace_chooser (plugin, seq_id, timestamp);
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

          if (sn_data->without_chooser)
            {
              guint32 timestamp = sn_startup_sequence_get_timestamp (sequence);

              finalize_app_startup (seq_id, sn_data->workspace, timestamp,
                                    plugin);
            }
          else
            {
              wsc_data = g_slice_new (struct ws_chooser_timeout_data);
              wsc_data->sn_id = g_strdup (seq_id);
              wsc_data->workspace = ws_count;
              wsc_data->plugin = plugin;

              priv->workspace_chooser_timeout =
                g_timeout_add_full (G_PRIORITY_DEFAULT,
                                WORKSPACE_CHOOSER_TIMEOUT,
                                workspace_chooser_timeout_cb,
                                wsc_data,
                                (GDestroyNotify)free_ws_chooser_timeout_data);
            }
        }
      break;
    case SN_MONITOR_EVENT_CANCELED:
      if (g_hash_table_lookup_extended (priv->sn_hash, seq_id, &key, &value))
        {
          SnHashData *sn_data = value;

          if (sn_data->mcw)
            {
              ActorPrivate *apriv = get_actor_private (sn_data->mcw);

              apriv->sn_in_progress = FALSE;
            }
        }
      g_hash_table_remove (priv->sn_hash, seq_id);
      break;
    }
}

static void
free_sn_hash_data (SnHashData *data)
{
  g_slice_free (SnHashData, data);
}

void
setup_startup_notification (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  Display                    *xdpy = mutter_plugin_get_xdisplay (plugin);

  /* startup notification */
  priv->sn_display = sn_display_new (xdpy, NULL, NULL);
  priv->sn_context = sn_monitor_context_new (priv->sn_display,
                                             DefaultScreen (xdpy),
                                             on_sn_monitor_event,
                                             plugin, NULL);

  priv->sn_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         g_free,
                                         (GDestroyNotify) free_sn_hash_data);
}

gboolean
startup_notification_should_map (MutterPlugin *plugin, MutterWindow *mcw,
                                 const gchar * sn_id)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gpointer                    key, value;

  if (!sn_id || !mcw)
    return TRUE;

  if (g_hash_table_lookup_extended (priv->sn_hash, sn_id,
                                    &key, &value))
    {
      SnHashData   *sn_data = value;
      gint          workspace_index;
      ActorPrivate *apriv = get_actor_private (mcw);

      apriv->sn_in_progress = TRUE;
      sn_data->mcw = mcw;

      workspace_index = sn_data->workspace;

      /*
       * If a workspace is set to a meaninful value, remove the
       * window from hash and move it to the appropriate WS.
       */
      if (workspace_index > -2)
        {
          if (!priv->desktop_switch_in_progress)
            {
              /*
               * If the WS effect is not in progress, we
               * reset the SN flag and remove the window from hash.
               *
               * We then move the window onto the appropriate workspace.
               */
              apriv->sn_in_progress = FALSE;

              g_hash_table_remove (priv->sn_hash, sn_id);
            }

          move_window_to_workspace (mcw, workspace_index, CurrentTime);

          if (priv->desktop_switch_in_progress)
            {
              return FALSE;
            }
        }
      else
        {
          /* Window has mapped, but no selection of workspace
           * (either explict by user, or implicity via timeout) has
           * taken place yet. We delay showing the actor.
           */
          return FALSE;
        }
    }

  return TRUE;
}

/*
 * Work through any windows in the SN hash and make sure they are mapped.
 *
 * (Called at the end of WS switching effect.)
 */
void
startup_notification_finalize (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gpointer                    key, value;
  GHashTableIter              iter;

  if (priv->desktop_switch_in_progress)
    return;

  g_hash_table_iter_init (&iter, priv->sn_hash);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      SnHashData *sn_data = value;
      gint        workspace_index;

      workspace_index = sn_data->workspace;

      /*
       * If a workspace is set to a meaninful value, remove the
       * window from hash and move it to the appropriate WS.
       */
      if (workspace_index > -2)
        {
          MutterWindow *mcw = sn_data->mcw;
          ActorPrivate *apriv = get_actor_private (mcw);

          apriv->sn_in_progress = FALSE;
          g_hash_table_remove (priv->sn_hash, key);
          g_hash_table_iter_init (&iter, priv->sn_hash);

          if (mcw)
            {
              MutterPluginClass *klass = MUTTER_PLUGIN_GET_CLASS (plugin);
              klass->map (plugin, mcw);
            }
        }
    }
}

void
spawn_app (MutterPlugin *plugin, const gchar *path, guint32 timestamp,
           gboolean without_chooser, gint workspace)
{
  MoblinNetbookPluginPrivate *priv    = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);
  Display                    *xdpy    = mutter_plugin_get_xdisplay (plugin);
  SnLauncherContext          *context = NULL;
  const gchar                *sn_id;
  gchar                     **argv;
  gint                        argc;
  SnHashData                 *sn_data = g_slice_new0 (SnHashData);
  GError                     *err = NULL;
  gint                        i;
  gboolean                    had_kbd_grab;

  if (!path)
    return;

  if (!g_shell_parse_argv (path, &argc, &argv, &err))
    {
      g_warning ("Error parsing command line: %s", err->message);
      g_clear_error (&err);
    }

  for (i = 0; i < argc; i++)
    {
      /* we don't support any % arguments in the desktop entry spec yet */
      if (argv[i][0] == '%')
        argv[i][0] = '\0';
    }

  context = sn_launcher_context_new (priv->sn_display, DefaultScreen (xdpy));

  sn_launcher_context_set_name (context, path);
  sn_launcher_context_set_description (context, path);
  sn_launcher_context_set_binary_name (context, path);

  sn_launcher_context_initiate (context,
                                "mutter-netbook-shell",
                                path, /* bin_name */
				timestamp);

  sn_id = sn_launcher_context_get_startup_id (context);

  sn_data->workspace = workspace;
  sn_data->without_chooser = without_chooser;

  g_hash_table_insert (priv->sn_hash, g_strdup (sn_id), sn_data);

  had_kbd_grab = release_keyboard (plugin, timestamp);

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

      if (had_kbd_grab)
        grab_keyboard (plugin, timestamp);

      g_hash_table_remove (priv->sn_hash, sn_id);
      sn_launcher_context_complete (context);
    }

  sn_launcher_context_unref (context);
  g_strfreev (argv);
}
