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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>

#include "moblin-netbook-chooser.h"
#include "moblin-netbook-panel.h"
#include "mnb-scale-group.h"

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <string.h>
#include <display.h>
#include <nbtk/nbtk.h>

#define WORKSPACE_CHOOSER_BORDER_LEFT   2
#define WORKSPACE_CHOOSER_BORDER_RIGHT  3
#define WORKSPACE_CHOOSER_BORDER_TOP    4
#define WORKSPACE_CHOOSER_BORDER_BOTTOM 4
#define WORKSPACE_CHOOSER_BORDER_PAD    8
#define WORKSPACE_CHOOSER_CELL_PAD      4
#define WORKSPACE_CHOOSER_LABEL_HEIGHT  40

#define WORKSPACE_CHOOSER_TIMEOUT 3000

#define MNBTK_SPINNER_ITERVAL 2000

#define MNBTK_SN_MAP_TIMEOUT 800

/******************************************************************
 * Workspace chooser
 */

typedef struct SnHashData    SnHashData;

struct SnHashData
{
  MutterWindow       *mcw;
  gint                workspace;
  SnMonitorEventType  state;
  guint               timeout_id;
  gboolean            without_chooser    : 1;
  gboolean            configured         : 1;
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

/*
 * Finalizes the sn arrangements for this application.
 *
 *  Creates the appropriate workspace if necessary, and moves the window to it.
 */
static void
finalize_app (const char * sn_id, gint workspace, guint32 timestamp,
              MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gpointer                    key, value;

  if (g_hash_table_lookup_extended (priv->sn_hash, sn_id, &key, &value))
    {
      SnHashData *sn_data = value;

      /*
       * Move the window to the requested workspace so that the switcher
       * workspace effect is triggered. If the window is supposed to go onto a
       * new workspace, we create it.
       *
       * If the WS effect is no longer in progress, we map the window.
       *
       */
      if (workspace == -2)
        {
          MetaScreen *screen = mutter_plugin_get_screen (plugin);

          meta_screen_append_new_workspace (screen, FALSE, timestamp);
        }

      move_window_to_workspace (sn_data->mcw, sn_data->workspace,
                                timestamp);
    }
}

struct map_timeout_data
{
  MutterPlugin *plugin;
  gchar        *sn_id;
};

/*
 * This timeout is installed when the application is configured; it handles the
 * awkward case where the start up sequence completes, but the application
 * reuses pre-existing window -- we try to activate that window.
 *
 * The timeout can also trigger for regular applications that are slow to map
 * their window; this should be OK, because when this happens we remove the app
 * from the hash here, so when the window finally maps normal processing should
 * ensue.
 */
static gboolean
sn_map_timeout_cb (gpointer data)
{
  GList *l;
  struct map_timeout_data    *map_data = data;
  MutterPlugin               *plugin   = map_data->plugin;
  gchar                      *sn_id    = map_data->sn_id;
  MoblinNetbookPluginPrivate *priv     = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gpointer                    key, value;

  if (g_hash_table_lookup_extended (priv->sn_hash, sn_id, &key, &value))
    {
      gboolean    removed = FALSE;
      SnHashData *sn_data = value;
      gchar      *s, *e;

      if (sn_data->state != SN_MONITOR_EVENT_COMPLETED)
        {
          /*
           * The timeout triggered before the sn process completed,
           * give it more time.
           */
          return TRUE;
        }

      /*
       * The startup id has the form:
       *
       *  launcher/app/numid-id-machine_timestamp.
       *
       * Isolate the app/numid part.
       */
      s = strchr (sn_id, '/');

      if (!s)
        {
          g_warning ("Unexpected form of sn_id [%s]\n", sn_id);
          goto finish;
        }

      s = g_strdup (s+1);

      e = strchr (s, '-');
      *e = 0;

      /*
       * Now iterate all windows and look for an sn_id that matches.
       */
      l = mutter_plugin_get_windows (map_data->plugin);

      while (l)
        {
          MutterWindow *mcw = l->data;
          MetaWindow   *mw  = mutter_window_get_meta_window (mcw);
          const gchar  *id  = meta_window_get_startup_id (mw);

          if (id && strstr (id, s))
            {
              MetaScreen    *screen  = mutter_plugin_get_screen (plugin);
              MetaWorkspace *active_workspace;
              MetaWorkspace *workspace;
              guint32        timestamp;

              timestamp = clutter_x11_get_current_event_time ();

              sn_data->mcw = mcw;

              /*
               * Apply the configuration settings to this application.
               */
              finalize_app (sn_id, sn_data->workspace, timestamp, plugin);

              /*
               * Remove it from the hash to ensure that it will not be handled
               * somewhere again.
               */
              removed = TRUE;
              g_hash_table_remove (priv->sn_hash, sn_id);

              /*
               * If the application is supposed to be on the same workspace
               * as the currently active one, then simply finalizing it does
               * not bring it to the forefront; do so now.
               */
              active_workspace = meta_screen_get_active_workspace (screen);
              workspace = meta_window_get_workspace (mw);

              if (!active_workspace || (active_workspace == workspace))
                {
                  meta_window_activate_with_workspace (mw, timestamp,
                                                       workspace);
                }
              break;
            }

          l = l->next;
        }

      g_free (s);

      if (!removed)
        g_hash_table_remove (priv->sn_hash, sn_id);
    }

 finish:
  g_free (map_data->sn_id);
  g_slice_free (struct map_timeout_data, data);

  return FALSE;
}

/*
 * Configures the application.
 *
 * This function is called in response to either the user interaction, or the
 * new workspace timeout. It sets the workspace where the application should
 * go, and if the window has already mapped, it initiates the map effect.
 */
static void
configure_app (const char *sn_id, gint workspace, MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gpointer                    key, value;

  if (workspace >= MAX_WORKSPACES)
    workspace = MAX_WORKSPACES - 1;

  if (g_hash_table_lookup_extended (priv->sn_hash, sn_id, &key, &value))
    {
      SnHashData *sn_data = value;

      /*
       * Update the workspace index if the start up has not been finalized.
       */
      if (!sn_data->configured)
        {
          sn_data->workspace = workspace;
          sn_data->configured = TRUE;
        }

      if (sn_data->mcw)
        {
          MutterPluginClass *klass = MUTTER_PLUGIN_GET_CLASS (plugin);
          klass->map (plugin, sn_data->mcw);
        }
      else if (sn_data->state == SN_MONITOR_EVENT_COMPLETED)
        {
          struct map_timeout_data *map_data;

          /*
           * If we do not have the window yet, install the map timeout.
           */
          map_data = g_slice_new (struct map_timeout_data);

          map_data->plugin = plugin;
          map_data->sn_id = g_strdup (sn_id);

          sn_data->timeout_id =
            g_timeout_add (MNBTK_SN_MAP_TIMEOUT, sn_map_timeout_cb, map_data);
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
  MetaScreen                 *screen = mutter_plugin_get_screen (plugin);
  MetaWorkspace              *workspace;
  const char                 *sn_id = wsg_data->sn_id;
  guint32                     timestamp;

  workspace = meta_screen_get_workspace_by_index (screen, indx);

  if (!workspace)
    {
      g_warning ("No workspace specified, %s:%d\n", __FILE__, __LINE__);
      return FALSE;
    }

  timestamp = clutter_x11_get_current_event_time ();

  hide_workspace_chooser (plugin, timestamp);

  configure_app (sn_id, wsg_data->workspace, plugin);

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
  const char                 *sn_id = kbd_data->sn_id;
  guint                       symbol = clutter_key_event_symbol (&event->key);
  gint                        indx;
  guint32                     timestamp;

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

  timestamp = clutter_x11_get_current_event_time ();

  hide_workspace_chooser (plugin, timestamp);

  configure_app (sn_id, indx, plugin);

  return TRUE;
}

/*
 * Creates a simple busy spinner; the returned actor has anchor point set
 * in its center for convenience.
 */
static ClutterActor *
make_spinner (void)
{
  static ClutterActor *tmp = NULL;

  ClutterActor *spinner = NULL;

  guint         s_w, s_h;

  static ClutterBehaviour *beh = NULL;

  if (tmp == NULL)
    {
      tmp = clutter_texture_new_from_file (PLUGIN_PKGDATADIR
                                               "/theme/generic/spinner.png", 
                                               NULL);

      if (!tmp)
        return NULL;

      clutter_actor_realize (tmp);
    }

  spinner = clutter_clone_new (tmp);

  clutter_actor_get_size (spinner, &s_w, &s_h);

  clutter_actor_set_anchor_point (spinner, s_w/2, s_h/2);

  if (!beh)
    {
      ClutterAlpha    *alpha;
      ClutterTimeline *timeline;

      timeline = clutter_timeline_new_for_duration (MNBTK_SPINNER_ITERVAL);
      clutter_timeline_set_loop (timeline, TRUE);

      alpha = clutter_alpha_new_full (timeline,
                                      CLUTTER_LINEAR);

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
  static ClutterActor *space_sel = NULL, *space_unsel = NULL, 
                      *thumb_sel = NULL, *thumb_unsel = NULL;
  ClutterActor *group, *bck, *label_actor;
  ClutterText *label;
  ClutterColor  white = { 0xff, 0xff, 0xff, 0xff };
  guint         l_w, l_h;


  if (space_sel == NULL && space_unsel == NULL
      && thumb_sel == NULL && thumb_unsel == NULL)
    {
      space_sel = clutter_texture_new_from_file 
                          (PLUGIN_PKGDATADIR
                           "/theme/chooser/space-selected.png", NULL);
      space_unsel = clutter_texture_new_from_file 
                          (PLUGIN_PKGDATADIR
                           "/theme/chooser/space-unselected.png", NULL);
      thumb_sel = clutter_texture_new_from_file 
                          (PLUGIN_PKGDATADIR
                           "/theme/chooser/thumb-selected.png",NULL);
      thumb_unsel = clutter_texture_new_from_file 
                          (PLUGIN_PKGDATADIR
                           "/theme/chooser/thumb-unselected.png", NULL);

      g_object_ref (space_sel);
      g_object_ref (space_unsel);
      g_object_ref (thumb_sel);
      g_object_ref (thumb_unsel);
    }

  group = clutter_group_new ();

  if (selected)
    bck = space_sel;
  else
    bck = space_unsel;

  if (bck)
    {
      ClutterActor *frame;

      frame = nbtk_texture_frame_new (CLUTTER_TEXTURE (bck), 11, 11, 11, 11);

      clutter_actor_set_size (frame, width, WORKSPACE_CHOOSER_LABEL_HEIGHT);

      clutter_container_add_actor (CLUTTER_CONTAINER (group), frame);
    }


  if (selected)
    bck =  clutter_clone_new (thumb_sel);
  else
    bck =  clutter_clone_new (thumb_unsel);

  if (bck)
    {
      clutter_actor_set_size (bck, width, height);
      clutter_actor_set_y (bck, WORKSPACE_CHOOSER_LABEL_HEIGHT +
                           WORKSPACE_CHOOSER_CELL_PAD);

      clutter_container_add_actor (CLUTTER_CONTAINER (group), bck);
    }

  if (text)
    {
      label_actor = clutter_text_new ();
      label = CLUTTER_TEXT (label_actor);

      clutter_text_set_font_name (label, "Sans 8");
      clutter_text_set_color (label, &white);
      clutter_text_set_text (label, text);

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
      group = mnb_scale_group_new ();

      clutter_actor_set_clip (group, 0, 0, screen_width, screen_height);

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
  const char                 *sn_id    = wsg_data->sn_id;
  guint32                     timestamp = clutter_x11_get_current_event_time ();

  hide_workspace_chooser (plugin, timestamp);

  configure_app (sn_id, wsg_data->workspace, wsg_data->plugin);

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
      MetaCompWindowType  type;
      gint                ws_indx;
      ClutterActor       *texture;
      ClutterActor       *clone;
      ClutterActor       *workspace = NULL;
      gint                x, y;

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
      clone   = clutter_clone_new (texture);

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
      gchar         *s = g_strdup_printf (_("Zone %d"), ws_count + 1);
      ClutterActor  *cell;

      struct ws_grid_cb_data * wsg_data =
        g_slice_new (struct ws_grid_cb_data);

      wsg_data->sn_id = g_strdup (sn_id);
      wsg_data->workspace = ws_count;
      wsg_data->plugin = plugin;

      cell = make_background (_(s),
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

  new_ws = make_background (_("New zone (0)"), cell_width, cell_height,
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
 * Creates and shows the workspace chooser.
 */
void
show_workspace_chooser (MutterPlugin *plugin,
                        const gchar * sn_id, guint32 timestamp)
{
  static ClutterActor *bck = NULL;
  ClutterActor *frame = NULL;

  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  ClutterActor               *overlay;
  ClutterActor               *switcher;
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
  
  if (bck == NULL)
    {
      bck = clutter_texture_new_from_file (PLUGIN_PKGDATADIR
                                           "/theme/chooser/background.png", 
                                           NULL);
      g_object_ref (bck);       /* extra ref to keep it around.. */
    }

  frame = nbtk_texture_frame_new (CLUTTER_TEXTURE (bck), 15, 15, 15, 15);

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

  label = clutter_text_new_full ("Sans 9",
                                 _("Choose zone for application:"), &label_clr);
  clutter_actor_realize (label);
  label_height = clutter_actor_get_height (label) + 3;

  grid = make_workspace_chooser (sn_id, &ws_count, plugin);
  clutter_actor_set_position (CLUTTER_ACTOR (grid), 0, label_height);

  clutter_container_add (CLUTTER_CONTAINER (switcher),
                         frame, label, grid, NULL);

#if 0
  if (priv->workspace_chooser)
    hide_workspace_chooser (plugin, timestamp);

  priv->workspace_chooser = switcher;
#endif

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

  moblin_netbook_stash_window_focus (plugin, timestamp);
  moblin_netbook_set_lowlight (plugin, TRUE);
}

void
hide_workspace_chooser (MutterPlugin *plugin, guint32 timestamp)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  //if (!priv->workspace_chooser)
  //  return;

  if (priv->workspace_chooser_timeout)
    {
      g_source_remove (priv->workspace_chooser_timeout);
      priv->workspace_chooser_timeout = 0;
    }

  moblin_netbook_set_lowlight (plugin, FALSE);
  moblin_netbook_unstash_window_focus (plugin, timestamp);

  // hide_panel (plugin);

  // clutter_actor_destroy (priv->workspace_chooser);
  // priv->workspace_chooser = NULL;
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
  guint32                         timestamp;

  timestamp = clutter_x11_get_current_event_time ();

  if (!priv->workspace_chooser_timeout)
    {
      g_message ("Workspace timeout triggered after user input, ignoring\n");
      return FALSE;
    }

  hide_workspace_chooser (plugin, timestamp);

  configure_app (wsc_data->sn_id, wsc_data->workspace, plugin);

  /* One off */
  return FALSE;
}

struct ws_chooser_map_data
{
  gchar        *sn_id;
  MutterPlugin *plugin;
};

/*
 * The start up notification handling.
 *
 * In essence it works like this:
 *
 * 1. We spawn an application.
 * 2. SN_MONITOR_EVENT_INITIATED
 * 3. SN_MONITOR_EVENT_COMPLETED: application started up successfully.
 * 4. MapNotify: maybe, see below.
 *
 * This we need to combine with user interactions and automatic timeout.
 *
 * a) Application should go onto a new workspace (unless 8 workspaces already).
 * b) Application should go onto existing workspace.
 *
 * So, in most cases we actually have to move the application to a different
 * workspace than it starts on.
 *
 * Additional caveats:
 *
 * i.  Workspace must not be created until we know application started,
 *     i.e., not before SN_MONITOR_EVENT_COMPLETED.
 *
 * ii. Application must be moved to the new workspace before we run the
 *     map effect, to avoid it flickering on the wrong workspace.
 *
 * The start up process is split into two phases: configuration and
 * finalization.
 *
 * Configuration (configure_app()):
 *
 *   Configuration is triggered either by user interaction or by the new
 *   workspace timeout. The application is tagged with the workspace needed,
 *   and marked as configured. If the window has already mapped, the map effect
 *   is triggered.
 *
 * Finalization (finalize_app()):
 *
 *   Finalization alsways happens after the application has been configured and
 *   is tirggered either by the window mapping effect (this in turn can be
 *   initiated directly by the map event, or synthesized by the configure_app()
 *   function.
 *
 *   During finalization a new workspace is created, if required, and the window
 *   is moved to this workspace.
 *
 * Corner cases:
 *
 * I. Application starts, but does not create a new window (e.g., a single
 *    instance application re-uses existing window, see bug 670). We need to
 *    avoid creating an empty workspace; we do this by delaying the workspace
 *    creation until the window has mapped. We also need to activate the
 *    application.
 *
 *    We install sn_map_timeout_cb () when the application is configured for
 *    a suitable period (500ms?), and when the timeout fires, check whether
 *    the application has mapped a window; if not, we query the start up ids
 *    of all currently running applications and try to find one that corresponds
 *    to ours. If we find it, we apply the configuration settings to this
 *    application.
 *
 */
static void
on_sn_monitor_event (SnMonitorEvent *event, gpointer data)
{
  MutterPlugin               *plugin = data;
  MoblinNetbookPluginPrivate *priv   = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  SnStartupSequence          *sequence;
  const char                 *seq_id = NULL, *bin_name = NULL;
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
        MetaScreen *screen    = mutter_plugin_get_screen (plugin);
        gint        n_ws      = meta_screen_get_n_workspaces (screen);
        gboolean    empty     = FALSE;
        SnHashData *sn_data   = g_slice_new0 (SnHashData);
        guint32     timestamp = sn_startup_sequence_get_timestamp (sequence);

        sn_data->state = SN_MONITOR_EVENT_INITIATED;
        sn_data->workspace = -2;

        if (n_ws == 1)
          {
            GList *l;
            empty = TRUE;
            for (l = mutter_get_windows (screen); l; l = l->next)
              {
                MutterWindow *m = l->data;
                MetaWindow   *w = mutter_window_get_meta_window (m);

                if (w)
                  {
                    MetaCompWindowType type = mutter_window_get_window_type (m);

                    if (type == META_COMP_WINDOW_NORMAL)
                      {
                        empty = FALSE;
                        break;
                      }
                  }
              }
          }

        if (empty)
          {
            sn_data->without_chooser = TRUE;
            sn_data->workspace = 0;
          }

          g_hash_table_insert (priv->sn_hash, g_strdup (seq_id), sn_data);

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
          struct ws_chooser_map_data     * wsc_map_data;

          sn_data->state = SN_MONITOR_EVENT_COMPLETED;

          wsc_map_data = g_slice_new (struct ws_chooser_map_data);
          wsc_map_data->plugin = plugin;
          wsc_map_data->sn_id = g_strdup (seq_id);

          /*
           * If the application has been configured already, we install
           * the map timeout here.
           */
          if (sn_data->configured)
            {
              struct map_timeout_data *map_data;

              /*
               * If we do not have the window yet, install the map timeout.
               */
              map_data = g_slice_new (struct map_timeout_data);

              map_data->plugin = plugin;
              map_data->sn_id = g_strdup (seq_id);

              sn_data->timeout_id =
                g_timeout_add (MNBTK_SN_MAP_TIMEOUT, sn_map_timeout_cb,
                               map_data);
            }

          if (sn_data->without_chooser)
            {
              /*
               * Configure the application
               */
              configure_app (seq_id, sn_data->workspace, plugin);
            }
          else
            {
              wsc_data = g_slice_new (struct ws_chooser_timeout_data);
              wsc_data->sn_id = g_strdup (seq_id);
              wsc_data->workspace = ws_count;
              wsc_data->plugin = plugin;

              /*
               * Start the timeout that automatically moves the application
               * to a new workspace if the user does not choose one manually.
               */
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
moblin_netbook_sn_setup (MutterPlugin *plugin)
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

/*
 * Public function called by the plugin map() handler to determine if the
 * window in question should be mapped.
 *
 * We have the following scenarios:
 *
 * 1. Window is not part of SN process, should be mapped, just return TRUE.
 *
 * 2. Window is part of SN process and has already been configured by
 *    configure_app(); we finalize the application, and if the desktop switch
 *    effect is not running, return TRUE, so that application window is mapped.
 *
 * 3. Window is part of SN process, but has not been configured yet; we return
 *    FALSE, to prevent window from mapping at this point (the map will be
 *    take care of once the window is actually configured).
 */
gboolean
moblin_netbook_sn_should_map (MutterPlugin *plugin, MutterWindow *mcw,
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
      ActorPrivate *apriv = get_actor_private (mcw);

      apriv->sn_in_progress = TRUE;
      sn_data->mcw = mcw;

      if (sn_data->timeout_id)
        {
          g_source_remove (sn_data->timeout_id);
          sn_data->timeout_id = 0;
        }

      if (sn_data->configured)
        {
          /* The start up process has already been configured before the
           * the window mapped; we need to re-run the finalize function to
           * take care of the actual workspace creation, etc.
           */
          guint32      timestamp;

          timestamp = clutter_x11_get_current_event_time ();

          finalize_app (sn_id, sn_data->workspace, timestamp, plugin);

          apriv->sn_in_progress = FALSE;

          g_hash_table_remove (priv->sn_hash, sn_id);
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
moblin_netbook_sn_finalize (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MutterPluginClass          *klass;
  gpointer                    key, value;
  GHashTableIter              iter;

  klass = MUTTER_PLUGIN_GET_CLASS (plugin);

  g_hash_table_iter_init (&iter, priv->sn_hash);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      SnHashData   *sn_data = value;
      gint          workspace_index;
      MutterWindow *mcw;

      workspace_index = sn_data->workspace;

      /*
       * If the app is already configured, trigger the map effect (this
       * takes care of everything else).
       */
      if (sn_data->configured && (mcw = sn_data->mcw))
        {
          ActorPrivate *apriv = get_actor_private (mcw);

          apriv->sn_in_progress = FALSE;
          g_hash_table_remove (priv->sn_hash, key);
          g_hash_table_iter_init (&iter, priv->sn_hash);

          klass->map (plugin, mcw);
        }
    }
}

void
moblin_netbook_spawn (MutterPlugin *plugin,
                      const  gchar *path,
                      guint32       timestamp,
                      gboolean      without_chooser,
                      gint          workspace)
{
  MoblinNetbookPluginPrivate *priv    = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  Display                    *xdpy    = mutter_plugin_get_xdisplay (plugin);
  SnLauncherContext          *context = NULL;
  const gchar                *sn_id;
  gchar                     **argv;
  gint                        argc;
  SnHashData                 *sn_data = g_slice_new0 (SnHashData);
  GError                     *err = NULL;
  gint                        i;

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
      g_hash_table_remove (priv->sn_hash, sn_id);
      sn_launcher_context_complete (context);
    }

  sn_launcher_context_unref (context);
  g_strfreev (argv);
}
