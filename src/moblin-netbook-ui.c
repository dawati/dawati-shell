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

/*******************************************************************
 * Workspace switcher, used to switch between existing workspaces.
 */

/*
 * Clone lifecycle house keeping.
 *
 * The workspace switcher and chooser hold clones of the window glx textures.
 * We need to ensure that when the master texture disappears, we remove the
 * clone as well. We do this via weak reference on the original object, and
 * so when the clone itself disappears, we need to remove the weak reference
 * from the master.
 */

void
switcher_origin_weak_notify (gpointer data, GObject *object)
{
  ClutterActor *clone = data;

  /*
   * The original MutterWindow destroyed; remove the weak reference the
   * we added to the clone referencing the original window, then
   * destroy the clone.
   */
  g_object_weak_unref (G_OBJECT (clone), switcher_clone_weak_notify, object);
  clutter_actor_destroy (clone);
}

void
switcher_clone_weak_notify (gpointer data, GObject *object)
{
  ClutterActor *origin = data;

  /*
   * Clone destroyed -- this function gets only called whent the clone
   * is destroyed while the original MutterWindow still exists, so remove
   * the weak reference we added on the origin for sake of the clone.
   */
  g_object_weak_unref (G_OBJECT (origin), switcher_origin_weak_notify, object);
}

void
hide_workspace_switcher (void)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  if (!priv->workspace_switcher)
    return;

  clutter_actor_destroy (priv->workspace_switcher);

  disable_stage (plugin);

  priv->workspace_switcher = NULL;
}

/*
 * Returns a container for n-th workspace.
 *
 * list -- WS containers; if the n-th container does not exist, it is
 *         appended.
 *
 * active -- index of the currently active workspace (to be highlighted)
 *
 */
ClutterActor *
ensure_nth_workspace (GList **list, gint n, gint active)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
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
      ClutterColor  background_clr = { 0, 0, 0, 0};
      ClutterColor  active_clr =     { 0xfd, 0xd9, 0x09, 0x7f};
      ClutterActor *background;


      /*
       * For non-expanded group, we use NutterScaleGroup container, which
       * allows us to apply scale to the workspace en mass.
       */
      group = nutter_scale_group_new ();

      /*
       * We need to add background, otherwise if the ws is empty, the group
       * will have size 0x0, and not respond to clicks.
       */
      if (i == active)
        background =  clutter_rectangle_new_with_color (&active_clr);
      else
        background =  clutter_rectangle_new_with_color (&background_clr);

      clutter_actor_set_size (background,
                              screen_width,
                              screen_height);

      clutter_container_add_actor (CLUTTER_CONTAINER (group), background);

      tmp = g_list_append (tmp, group);

      ++i;
    }

  g_assert (tmp);

  *list = g_list_concat (*list, tmp);

  return g_list_last (*list)->data;
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

/*
 * Creates an iconic representation of the workspace with the label provided.
 *
 * We use the custom NutterWsIcon actor, which automatically handles layout
 * when the icon is resized.
 */
static ClutterActor *
make_workspace_label (const gchar *text)
{
  NutterWsIcon *icon;
  ClutterActor *actor;
  ClutterColor  b_clr = { 0x44, 0x44, 0x44, 0xff };
  ClutterColor  f_clr = { 0xff, 0xff, 0xff, 0xff };

  actor = nutter_ws_icon_new ();
  icon  = NUTTER_WS_ICON (actor);

  clutter_actor_set_size (actor, WORKSPACE_CELL_WIDTH, WORKSPACE_CELL_HEIGHT);

  nutter_ws_icon_set_font_name (icon, "Sans 16");
  nutter_ws_icon_set_text (icon, text);
  nutter_ws_icon_set_color (icon, &b_clr);
  nutter_ws_icon_set_border_width (icon, 3);
  nutter_ws_icon_set_text_color (icon, &f_clr);
  nutter_ws_icon_set_border_color (icon, &f_clr);

  return actor;
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
  gint           ws_count, ws_max_windows = 0;
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

  return CLUTTER_ACTOR (table);
}

/*
 * Constructs and shows the workspace switcher actor.
 */
void
show_workspace_switcher (void)
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
    hide_workspace_switcher ();

  priv->workspace_switcher = switcher;

  overlay = mutter_plugin_get_overlay_group (plugin);
  clutter_container_add_actor (CLUTTER_CONTAINER (overlay), switcher);

  clutter_actor_get_size (switcher, &switcher_width, &switcher_height);
  clutter_actor_set_size (background, switcher_width, switcher_height);

  panel_height = clutter_actor_get_height (priv->panel);
  panel_y      = clutter_actor_get_y (priv->panel);
  clutter_actor_set_position (switcher, 0, panel_height);

  mutter_plugin_set_stage_reactive (plugin, TRUE);
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

