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

#define MUTTER_BUILDING_PLUGIN 1

#include "moblin-netbook.h"
#include "moblin-netbook-ui.h"
#include "moblin-netbook-chooser.h"
#include "moblin-netbook-panel.h"
#include "nbtk-behaviour-bounce.h"

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gmodule.h>
#include <string.h>

#include <compositor-mutter.h>
#include <display.h>
#include <prefs.h>

#define DESTROY_TIMEOUT             150
#define MINIMIZE_TIMEOUT            250
#define MAXIMIZE_TIMEOUT            250
#define MAP_TIMEOUT                 350
#define SWITCH_TIMEOUT              400
#define PANEL_SLIDE_TIMEOUT         150
#define PANEL_SLIDE_THRESHOLD       2
#define WS_SWITCHER_SLIDE_TIMEOUT   250
#define WS_SWITCHER_SLIDE_THRESHOLD 3
#define ACTOR_DATA_KEY "MCCP-moblin-netbook-actor-data"

static GQuark actor_data_quark = 0;

static gboolean do_init  (const char *params);
static void     minimize (MutterWindow *actor);
static void     map      (MutterWindow *actor);
static void     destroy  (MutterWindow *actor);
static void     maximize (MutterWindow *actor,
                          gint x, gint y, gint width, gint height);
static void     unmaximize (MutterWindow *actor,
                            gint x, gint y, gint width, gint height);
static void     switch_workspace (const GList **actors, gint from, gint to,
                                  MetaMotionDirection direction);
static void     kill_effect (MutterWindow *actor, gulong event);
static gboolean xevent_filter (XEvent *xev);
static gboolean reload (const char *params);

/*
 * Create the plugin struct; function pointers initialized in
 * g_module_check_init().
 */
MUTTER_DECLARE_PLUGIN ();

/*
 * Actor private data accessor
 */
static void
free_actor_private (gpointer data)
{
  if (G_LIKELY (data != NULL))
    g_slice_free (ActorPrivate, data);
}

ActorPrivate *
get_actor_private (MutterWindow *actor)
{
  ActorPrivate *priv = g_object_get_qdata (G_OBJECT (actor), actor_data_quark);

  if (G_UNLIKELY (actor_data_quark == 0))
    actor_data_quark = g_quark_from_static_string (ACTOR_DATA_KEY);

  if (G_UNLIKELY (!priv))
    {
      priv = g_slice_new0 (ActorPrivate);

      g_object_set_qdata_full (G_OBJECT (actor),
                               actor_data_quark, priv,
                               free_actor_private);
    }

  return priv;
}

static void
on_desktop_pre_paint (ClutterActor *actor,
                      gpointer      user_data)
{
  MutterPlugin      *plugin = mutter_get_plugin ();
  PluginPrivate     *priv   = plugin->plugin_private;
  ClutterActor      *parent_texture;
  gint               x_1, y_1, x_2, y_2;
  ClutterColor       col = { 0xff, 0xff, 0xff, 0xff };
  CoglHandle         cogl_texture;
  ClutterFixed       t_w, t_h;
  guint              tex_width, tex_height;
  guint              w, h;

  clutter_actor_get_size (priv->parallax_tex, &w, &h);

  cogl_translate (priv->parallax_paint_offset - w/4 , 0 , 0);

  cogl_texture
       = clutter_texture_get_cogl_texture (CLUTTER_TEXTURE(priv->parallax_tex));

  if (cogl_texture == COGL_INVALID_HANDLE)
    return;

  col.alpha = clutter_actor_get_paint_opacity (actor);
  cogl_color (&col);

  tex_width = cogl_texture_get_width (cogl_texture);
  tex_height = cogl_texture_get_height (cogl_texture);

  t_w = CFX_QDIV (CLUTTER_INT_TO_FIXED (w),
                  CLUTTER_INT_TO_FIXED (tex_width));
  t_h = CFX_QDIV (CLUTTER_INT_TO_FIXED (h),
                  CLUTTER_INT_TO_FIXED (tex_height));

  /* Parent paint translated us into position */
  cogl_texture_rectangle (cogl_texture, 0, 0,
			  CLUTTER_INT_TO_FIXED (w),
			  CLUTTER_INT_TO_FIXED (h),
			  0, 0, t_w, t_h);

  g_signal_stop_emission_by_name (actor, "paint");
}

static void
on_switch_workspace_effect_complete (ClutterActor *group, gpointer data)
{
  MutterPlugin   *plugin = mutter_get_plugin ();
  PluginPrivate  *ppriv  = plugin->plugin_private;
  GList          *l      = *((GList**)data);
  MutterWindow   *actor_for_cb = l->data;

  while (l)
    {
      ClutterActor *a    = l->data;
      MutterWindow *mcw  = MUTTER_WINDOW (a);
      ActorPrivate *priv = get_actor_private (mcw);

      if (priv->orig_parent)
        {
          clutter_actor_reparent (a, priv->orig_parent);
          priv->orig_parent = NULL;
        }

      l = l->next;
    }

  clutter_actor_destroy (ppriv->desktop1);
  clutter_actor_destroy (ppriv->desktop2);
  clutter_actor_destroy (ppriv->d_overlay);

  ppriv->actors = NULL;
  ppriv->tml_switch_workspace0 = NULL;
  ppriv->tml_switch_workspace1 = NULL;
  ppriv->desktop1 = NULL;
  ppriv->desktop2 = NULL;
  ppriv->desktop_switch_in_progress = FALSE;

  startup_notification_finalize ();

  mutter_plugin_effect_completed (plugin, actor_for_cb,
                                  MUTTER_PLUGIN_SWITCH_WORKSPACE);
}

static void
on_workspace_frame_change (ClutterTimeline *timeline,
                           gint             frame_num,
                           gpointer         data)
{
  MutterPlugin   *plugin = mutter_get_plugin ();
  PluginPrivate  *priv  = plugin->plugin_private;

  priv->parallax_paint_offset += GPOINTER_TO_INT (data) * -1;
}

static void
switch_workspace (const GList **actors, gint from, gint to,
                  MetaMotionDirection direction)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *ppriv  = plugin->plugin_private;
  GList         *l;
  gint           n_workspaces;
  ClutterActor  *workspace_slider0  = clutter_group_new ();
  ClutterActor  *workspace_slider1  = clutter_group_new ();
  ClutterActor  *indicator_group  = clutter_group_new ();
  ClutterActor  *stage, *label, *rect, *window_layer, *overlay_layer;
  gint           to_x, to_y, from_x = 0, from_y = 0;
  gint           para_dir = 2;
  ClutterColor   white = { 0xff, 0xff, 0xff, 0xff };
  ClutterColor   black = { 0x33, 0x33, 0x33, 0xff };
  gint           screen_width;
  gint           screen_height;
  guint		 indicator_width, indicator_height;

  if (from == to)
    {
      mutter_plugin_effect_completed (plugin, NULL,
				      MUTTER_PLUGIN_SWITCH_WORKSPACE);
      return;
    }

  stage = mutter_plugin_get_stage (plugin);

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  window_layer = mutter_plugin_get_window_group (plugin);
  overlay_layer = mutter_plugin_get_overlay_group (plugin);

  clutter_container_add_actor (CLUTTER_CONTAINER (window_layer),
			       workspace_slider0);
  clutter_container_add_actor (CLUTTER_CONTAINER (window_layer),
			       workspace_slider1);
  clutter_container_add_actor (CLUTTER_CONTAINER (overlay_layer),
			       indicator_group);

  n_workspaces = g_list_length (plugin->work_areas);

  for (l = g_list_last (*((GList**) actors)); l != NULL; l = l->prev)
    {
      MutterWindow *mcw  = l->data;
      ActorPrivate *priv = get_actor_private (mcw);
      ClutterActor *a    = CLUTTER_ACTOR (mcw);
      gint          workspace;

      /* We don't care about minimized windows */
      if (!mutter_window_showing_on_its_workspace (mcw))
	continue;

      workspace = mutter_window_get_workspace (mcw);

      if (workspace == to || workspace == from)
        {
	  ClutterActor *slider =
	    workspace == to ? workspace_slider1 : workspace_slider0;
          gint x, y;
          guint w, h;

          clutter_actor_get_position (a, &x, &y);
          clutter_actor_get_size (a, &w, &h);

          priv->orig_parent = clutter_actor_get_parent (a);

          clutter_actor_reparent (a, slider);

          /*
           * If the window is in SN flux, we will hide it; we still need to
           * reparent it, otherwise we screw up the stack order.
           *
           * The map effect will take care of showing the actor again.
           */
          if (priv->sn_in_progress)
            clutter_actor_hide (a);
          else
            clutter_actor_show_all (a);

          clutter_actor_raise_top (a);
        }
      else if (workspace < 0)
        {
          /* Sticky window */
          priv->orig_parent = NULL;
        }
      else
        {
          /* Window on some other desktop */
          clutter_actor_hide (a);
          priv->orig_parent = NULL;
        }
    }

  /* Make arrow indicator */
  rect = clutter_rectangle_new ();
  clutter_rectangle_set_color (CLUTTER_RECTANGLE (rect), &white);
  clutter_container_add_actor (CLUTTER_CONTAINER (indicator_group), rect);

  label = clutter_label_new ();
  clutter_label_set_font_name (CLUTTER_LABEL (label), "Sans Bold 148");
  clutter_label_set_color (CLUTTER_LABEL (label), &black);
  clutter_container_add_actor (CLUTTER_CONTAINER (indicator_group), label);

  clutter_actor_set_size (rect,
                          clutter_actor_get_width (label),
                          clutter_actor_get_height (label));

  ppriv->actors  = (GList **)actors;
  ppriv->desktop1 = workspace_slider0;
  ppriv->desktop2 = workspace_slider1;
  ppriv->d_overlay = indicator_group;

  switch (direction)
    {
    case META_MOTION_UP:
      clutter_label_set_text (CLUTTER_LABEL (label), "\342\206\221");

      to_x = 0;
      to_y = -screen_height;
      break;

    case META_MOTION_DOWN:
      clutter_label_set_text (CLUTTER_LABEL (label), "\342\206\223");

      to_x = 0;
      to_y = -screen_height;
      break;

    case META_MOTION_LEFT:
      clutter_label_set_text (CLUTTER_LABEL (label), "\342\206\220");

      to_x = -screen_width * -1;
      to_y = 0;

      para_dir = -2;
      break;

    case META_MOTION_RIGHT:
      clutter_label_set_text (CLUTTER_LABEL (label), "\342\206\222");

      to_x = -screen_width;
      to_y = 0;
      break;

    default:
      break;
    }

  /* dest group offscreen and on top */
  clutter_actor_set_position (workspace_slider1, to_x * -1, to_y);
  clutter_actor_raise_top (workspace_slider1);

  /* center arrow */
  clutter_actor_get_size (indicator_group, &indicator_width, &indicator_height);
  clutter_actor_set_position (indicator_group,
			      (screen_width - indicator_width) / 2,
			      (screen_height - indicator_height) / 2);

  ppriv->desktop_switch_in_progress = TRUE;

  /* workspace were going too */
  ppriv->tml_switch_workspace1 =
    clutter_effect_move (ppriv->switch_workspace_effect, workspace_slider1,
                         0, 0,
                         on_switch_workspace_effect_complete,
                         actors);
  /* coming from */
  ppriv->tml_switch_workspace0 =
    clutter_effect_move (ppriv->switch_workspace_effect, workspace_slider0,
                         to_x, to_y,
                         NULL, NULL);

  /* arrow */
  clutter_effect_fade (ppriv->switch_workspace_arrow_effect, indicator_group,
                       0,
                       NULL, NULL);

  /* desktop parallax */
  g_signal_connect (ppriv->tml_switch_workspace1,
                    "new-frame",
                    G_CALLBACK (on_workspace_frame_change),
                    GINT_TO_POINTER(para_dir));
}

/*
 * Minimize effect completion callback; this function restores actor state, and
 * calls the manager callback function.
 */
static void
on_minimize_effect_complete (ClutterActor *actor, gpointer data)
{
  /*
   * Must reverse the effect of the effect; must hide it first to ensure
   * that the restoration will not be visible.
   */
  MutterPlugin *plugin = mutter_get_plugin ();
  ActorPrivate *apriv;
  MutterWindow *mcw = MUTTER_WINDOW (actor);

  apriv = get_actor_private (MUTTER_WINDOW (actor));
  apriv->tml_minimize = NULL;

  clutter_actor_hide (actor);

  clutter_actor_set_scale (actor, 1.0, 1.0);
  clutter_actor_move_anchor_point_from_gravity (actor,
                                                CLUTTER_GRAVITY_NORTH_WEST);

  /* Now notify the manager that we are done with this effect */
  mutter_plugin_effect_completed (plugin, mcw,
                                      MUTTER_PLUGIN_MINIMIZE);
}

/*
 * Simple minimize handler: it applies a scale effect (which must be reversed on
 * completion).
 */
static void
minimize (MutterWindow *mcw)

{
  MutterPlugin      *plugin = mutter_get_plugin ();
  PluginPrivate     *priv   = plugin->plugin_private;
  MetaCompWindowType type;
  ClutterActor      *actor  = CLUTTER_ACTOR (mcw);

  type = mutter_window_get_window_type (mcw);

  if (type == META_COMP_WINDOW_NORMAL)
    {
      ActorPrivate *apriv = get_actor_private (mcw);

      apriv->is_minimized = TRUE;

      clutter_actor_move_anchor_point_from_gravity (actor,
                                                    CLUTTER_GRAVITY_CENTER);

      apriv->tml_minimize = clutter_effect_scale (priv->minimize_effect,
                                                  actor,
                                                  0.0,
                                                  0.0,
                                                  (ClutterEffectCompleteFunc)
                                                  on_minimize_effect_complete,
                                                  NULL);
    }
  else
    mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MINIMIZE);
}

/*
 * Minimize effect completion callback; this function restores actor state, and
 * calls the manager callback function.
 */
static void
on_maximize_effect_complete (ClutterActor *actor, gpointer data)
{
  /*
   * Must reverse the effect of the effect.
   */
  MutterPlugin *plugin = mutter_get_plugin ();
  MutterWindow *mcw    = MUTTER_WINDOW (actor);
  ActorPrivate *apriv  = get_actor_private (mcw);

  apriv->tml_maximize = NULL;

  clutter_actor_set_scale (actor, 1.0, 1.0);
  clutter_actor_move_anchor_point_from_gravity (actor,
                                                CLUTTER_GRAVITY_NORTH_WEST);

  /* Now notify the manager that we are done with this effect */
  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAXIMIZE);
}

/*
 * The Nature of Maximize operation is such that it is difficult to do a visual
 * effect that would work well. Scaling, the obvious effect, does not work that
 * well, because at the end of the effect we end up with window content bigger
 * and differently laid out than in the real window; this is a proof concept.
 *
 * (Something like a sound would be more appropriate.)
 */
static void
maximize (MutterWindow *mcw,
          gint end_x, gint end_y, gint end_width, gint end_height)
{
  MutterPlugin       *plugin = mutter_get_plugin ();
  PluginPrivate      *priv   = plugin->plugin_private;
  MetaCompWindowType  type;
  ClutterActor       *actor  = CLUTTER_ACTOR (mcw);

  gdouble  scale_x  = 1.0;
  gdouble  scale_y  = 1.0;
  gint     anchor_x = 0;
  gint     anchor_y = 0;

  type = mutter_window_get_window_type (mcw);

  if (type == META_COMP_WINDOW_NORMAL)
    {
      ActorPrivate *apriv = get_actor_private (mcw);
      guint width, height;
      gint  x, y;

      apriv->is_maximized = TRUE;

      clutter_actor_get_size (actor, &width, &height);
      clutter_actor_get_position (actor, &x, &y);

      /*
       * Work out the scale and anchor point so that the window is expanding
       * smoothly into the target size.
       */
      scale_x = (gdouble)end_width / (gdouble) width;
      scale_y = (gdouble)end_height / (gdouble) height;

      anchor_x = (gdouble)(x - end_x)*(gdouble)width /
        ((gdouble)(end_width - width));
      anchor_y = (gdouble)(y - end_y)*(gdouble)height /
        ((gdouble)(end_height - height));

      clutter_actor_move_anchor_point (actor, anchor_x, anchor_y);

      apriv->tml_maximize = clutter_effect_scale (priv->maximize_effect,
                                                  actor,
                                                  scale_x,
                                                  scale_y,
                                                  (ClutterEffectCompleteFunc)
                                                  on_maximize_effect_complete,
                                                  NULL);

      return;
    }

  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAXIMIZE);
}

/*
 * See comments on the maximize() function.
 *
 * (Just a skeleton code.)
 */
static void
unmaximize (MutterWindow *mcw,
            gint end_x, gint end_y, gint end_width, gint end_height)
{
  MutterPlugin       *plugin = mutter_get_plugin ();
  MetaCompWindowType  type;

  type = mutter_window_get_window_type (mcw);

  if (type == META_COMP_WINDOW_NORMAL)
    {
      ActorPrivate *apriv = get_actor_private (mcw);

      apriv->is_maximized = FALSE;
    }

  /* Do this conditionally, if the effect requires completion callback. */
  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_UNMAXIMIZE);
}

/*
static void
on_map_effect_complete (ClutterActor *actor, gpointer data)
{
*/
static void
on_map_effect_complete (ClutterTimeline *timeline, gpointer data)
{
  /*
   * Must reverse the effect of the effect.
   */
  MutterPlugin *plugin = mutter_get_plugin ();
  ClutterActor *actor  = CLUTTER_ACTOR(data);
  MutterWindow *mcw    = MUTTER_WINDOW (actor);
  ActorPrivate *apriv  = get_actor_private (mcw);

  apriv->tml_map = NULL;

  clutter_actor_move_anchor_point_from_gravity (actor,
                                                CLUTTER_GRAVITY_NORTH_WEST);

  /* Now notify the manager that we are done with this effect */
  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
}

/*
 * Simple map handler: it applies a scale effect which must be reversed on
 * completion).
 */
static void
map (MutterWindow *mcw)
{
  MutterPlugin       *plugin = mutter_get_plugin ();
  PluginPrivate      *priv   = plugin->plugin_private;
  MetaCompWindowType  type;
  ClutterActor       *actor  = CLUTTER_ACTOR (mcw);

  type = mutter_window_get_window_type (mcw);

  if (type == META_COMP_WINDOW_DESKTOP
      && priv->parallax_tex != NULL ) /* FIXME */
    {
      gint screen_width, screen_height;

      /*
       * FIXME -- the way it currently works means we still have a fullscreen
       * GLX texture in place which serves no purpose. We should make this work
       * without needing the desktop window. The parallax texture could simply
       * be placed directly on stage, underneath the Mutter windows group.
       */
      mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

      clutter_actor_set_size (priv->parallax_tex,
                              screen_width * 8,
                              screen_height);

      clutter_actor_set_parent (priv->parallax_tex, actor);

      g_signal_connect (actor,
                        "paint", G_CALLBACK (on_desktop_pre_paint),
                        NULL);

      mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
      return;
    }

  if (type == META_COMP_WINDOW_NORMAL)
    {
      ActorPrivate *apriv = get_actor_private (mcw);
      MetaWindow   *mw = mutter_window_get_meta_window (mcw);
      gint          workspace_index = -2;

      if (mw)
        {
          const char *sn_id = meta_window_get_startup_id (mw);

          if (!startup_notification_should_map (mcw, sn_id))
            return;
        }

      clutter_actor_move_anchor_point_from_gravity (actor,
                                                    CLUTTER_GRAVITY_CENTER);

      clutter_actor_set_scale (actor, 0.0, 0.0);
      clutter_actor_show (actor);
      /*
      apriv->tml_map = clutter_effect_scale (priv->map_effect,
                                             actor,
                                             1.0,
                                             1.0,
                                             (ClutterEffectCompleteFunc)
                                             on_map_effect_complete,
                                             NULL);
      */
      apriv->tml_map = nbtk_bounce_scale (actor, MAP_TIMEOUT);

      g_signal_connect (apriv->tml_map, "completed",
                        G_CALLBACK (on_map_effect_complete), actor);

      apriv->is_minimized = FALSE;
    }
  else
    mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
}

/*
 * Destroy effect completion callback; this is a simple effect that requires no
 * further action than notifying the manager that the effect is completed.
 */
static void
on_destroy_effect_complete (ClutterActor *actor, gpointer data)
{
  MutterPlugin *plugin = mutter_get_plugin ();
  MutterWindow *mcw    = MUTTER_WINDOW (actor);
  ActorPrivate *apriv  = get_actor_private (mcw);

  apriv->tml_destroy = NULL;

  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_DESTROY);
}

static void
check_for_empty_workspaces (gint workspace, MutterWindow *ignore)
{
  MutterPlugin   *plugin = mutter_get_plugin ();
  PluginPrivate  *priv   = plugin->plugin_private;
  MetaScreen     *screen = mutter_plugin_get_screen (plugin);
  gboolean        workspace_empty = TRUE;
  GList          *l;

  l = mutter_get_windows (screen);
  while (l)
    {
      MutterWindow *m = l->data;

      if (m != ignore)
        {
          gint w = mutter_window_get_workspace (m);

          if (w == workspace)
            {
              workspace_empty = FALSE;
              break;
            }
        }

      l = l->next;
    }

  if (workspace_empty)
    {
      MetaWorkspace *mws;
      MetaDisplay   *display;
      guint32        timestamp;

      display = meta_screen_get_display (screen);
      timestamp = meta_display_get_current_time_roundtrip (display);

      mws = meta_screen_get_workspace_by_index (screen, workspace);

      meta_screen_remove_workspace (screen, mws, timestamp);
    }
}

/*
 * Simple TV-out like effect.
 */
static void
destroy (MutterWindow *mcw)
{
  MutterPlugin       *plugin = mutter_get_plugin ();
  PluginPrivate      *priv   = plugin->plugin_private;
  MetaScreen         *screen;
  MetaCompWindowType  type;
  ClutterActor       *actor  = CLUTTER_ACTOR (mcw);
  gint                workspace;
  gboolean            workspace_empty = TRUE;

  type      = mutter_window_get_window_type (mcw);
  workspace = mutter_window_get_workspace (mcw);
  screen    = mutter_plugin_get_screen (plugin);

  if (type == META_COMP_WINDOW_NORMAL)
    {
      ActorPrivate *apriv  = get_actor_private (mcw);

      clutter_actor_move_anchor_point_from_gravity (actor,
                                                    CLUTTER_GRAVITY_CENTER);

      apriv->tml_destroy = clutter_effect_scale (priv->destroy_effect,
                                                 actor,
                                                 0.0,
                                                 0.0,
                                                 (ClutterEffectCompleteFunc)
                                                 on_destroy_effect_complete,
                                                 NULL);
    }
  else
    mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_DESTROY);

  check_for_empty_workspaces (workspace, mcw);
}

/*
 * Use this function to disable stage input
 *
 * Used by the completion callback for the panel in/out effects
 */
void
disable_stage (MutterPlugin *plugin, guint32 timestamp)
{
  PluginPrivate *priv = plugin->plugin_private;

  mutter_plugin_set_stage_input_region (plugin, priv->input_region);

  if (priv->keyboard_grab)
    {
      if (timestamp == CurrentTime)
        {
          MetaScreen  *screen  = mutter_plugin_get_screen (plugin);
          MetaDisplay *display = meta_screen_get_display (screen);

          timestamp = meta_display_get_current_time_roundtrip (display);
        }

      Display *xdpy = mutter_plugin_get_xdisplay (plugin);

      XUngrabKeyboard (xdpy, timestamp);
      XSync (xdpy, False);
      priv->keyboard_grab = FALSE;
    }
}

void
enable_stage (MutterPlugin *plugin, guint32 timestamp)
{
  PluginPrivate *priv   = plugin->plugin_private;
  MetaScreen    *screen = mutter_plugin_get_screen (plugin);
  Display       *xdpy   = mutter_plugin_get_xdisplay (plugin);
  ClutterActor  *stage  = mutter_get_stage_for_screen (screen);
  Window         xwin   = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

  mutter_plugin_set_stage_reactive (plugin, TRUE);

  if (!priv->keyboard_grab)
    {
      if (timestamp == CurrentTime)
        {
          MetaDisplay *display = meta_screen_get_display (screen);

          timestamp = meta_display_get_current_time_roundtrip (display);
        }

      if (Success == XGrabKeyboard (xdpy, xwin, True,
                                    GrabModeAsync, GrabModeAsync, timestamp))
        {
          priv->keyboard_grab = TRUE;
        }
      else
        g_warning ("Stage keyboard grab failed!\n");
    }
}

static gboolean
xevent_filter (XEvent *xev)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv  = plugin->plugin_private;

  sn_display_process_event (priv->sn_display, xev);

  clutter_x11_handle_event (xev);

  return FALSE;
}

static void
kill_effect (MutterWindow *mcw, gulong event)
{
  MutterPlugin *plugin = mutter_get_plugin ();
  ActorPrivate *apriv;
  ClutterActor *actor = CLUTTER_ACTOR (mcw);

  if (event & MUTTER_PLUGIN_SWITCH_WORKSPACE)
    {
      PluginPrivate *ppriv  = plugin->plugin_private;

      if (ppriv->tml_switch_workspace0)
        {
          clutter_timeline_stop (ppriv->tml_switch_workspace0);
          clutter_timeline_stop (ppriv->tml_switch_workspace1);
          on_switch_workspace_effect_complete (ppriv->desktop1, ppriv->actors);
        }

      if (!(event & ~MUTTER_PLUGIN_SWITCH_WORKSPACE))
        {
          /* Workspace switch only, nothing more to do */
          return;
        }
    }

  apriv = get_actor_private (mcw);

  if ((event & MUTTER_PLUGIN_MINIMIZE) && apriv->tml_minimize)
    {
      clutter_timeline_stop (apriv->tml_minimize);
      on_minimize_effect_complete (actor, NULL);
    }

  if ((event & MUTTER_PLUGIN_MAXIMIZE) && apriv->tml_maximize)
    {
      clutter_timeline_stop (apriv->tml_maximize);
      on_maximize_effect_complete (actor, NULL);
    }

  if ((event & MUTTER_PLUGIN_MAP) && apriv->tml_map)
    {
      clutter_timeline_stop (apriv->tml_map);
      on_map_effect_complete (apriv->tml_map, actor);
    }

  if ((event & MUTTER_PLUGIN_DESTROY) && apriv->tml_destroy)
    {
      clutter_timeline_stop (apriv->tml_destroy);
      on_destroy_effect_complete (actor, NULL);
    }
}


const gchar * g_module_check_init (GModule *module);
const gchar *
g_module_check_init (GModule *module)
{
  MutterPlugin *plugin = mutter_get_plugin ();

  /* Human readable name (for use in UI) */
  plugin->name = "Moblin Netbook",

  /* Plugin load time initialiser */
  plugin->do_init = do_init;

  /* Effect handlers */
  plugin->minimize         = minimize;
  plugin->destroy          = destroy;
  plugin->map              = map;
  plugin->maximize         = maximize;
  plugin->unmaximize       = unmaximize;
  plugin->switch_workspace = switch_workspace;
  plugin->kill_effect      = kill_effect;
  plugin->xevent_filter    = xevent_filter;

  /* The reload handler */
  plugin->reload           = reload;

  return NULL;
}

static void
on_panel_out_effect_complete (ClutterActor *panel, gpointer data)
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;

  priv->panel_out_in_progress = FALSE;

  enable_stage (plugin, CurrentTime);
}

/*
 * Handles input events on stage.
 *
 * NB: used both during capture and bubble stages.
 */
static gboolean
stage_input_cb (ClutterActor *stage, ClutterEvent *event, gpointer data)
{
  gboolean capture = GPOINTER_TO_INT (data);

  if ((capture && event->type == CLUTTER_MOTION) ||
      (!capture && event->type == CLUTTER_BUTTON_PRESS))
    {
      gint           event_y, event_x;
      MutterPlugin  *plugin = mutter_get_plugin ();
      PluginPrivate *priv   = plugin->plugin_private;

      if (event->type == CLUTTER_MOTION)
        {
          event_x = ((ClutterMotionEvent*)event)->x;
          event_y = ((ClutterMotionEvent*)event)->y;
        }
      else
        {
          event_x = ((ClutterButtonEvent*)event)->x;
          event_y = ((ClutterButtonEvent*)event)->y;
        }

      if (priv->panel_out_in_progress || priv->panel_back_in_progress)
        return FALSE;

      if (!capture && event->type == CLUTTER_BUTTON_PRESS)
        {
          if (priv->workspace_switcher)
            hide_workspace_switcher ();

          if (priv->launcher)
            clutter_actor_hide (priv->launcher);
        }

      if (priv->panel_out &&
          (event->type == CLUTTER_BUTTON_PRESS ||
           (!priv->switcher && !priv->workspace_switcher &&
            !CLUTTER_ACTOR_IS_VISIBLE (priv->launcher))))
        {
          guint height = clutter_actor_get_height (priv->panel_shadow);

          if (event_y > (gint)height)
            {
              hide_panel ();
            }
        }
      else if (event_y < PANEL_SLIDE_THRESHOLD)
        {
          gint  x = clutter_actor_get_x (priv->panel);

          priv->panel_out_in_progress  = TRUE;
          clutter_effect_move (priv->panel_slide_effect,
                               priv->panel, x, 0,
                               on_panel_out_effect_complete,
                               NULL);

          priv->panel_out = TRUE;
        }
    }
  else if (event->type == CLUTTER_KEY_PRESS)
    {
      ClutterKeyEvent *kev = (ClutterKeyEvent *) event;

      g_print ("*** key press event (key:%c) ***\n",
	       clutter_key_event_symbol (kev));

    }

  return FALSE;
}

static void
setup_parallax_effect (void)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv  = plugin->plugin_private;
  MetaScreen    *screen = mutter_plugin_get_screen (plugin);
  MetaDisplay   *display = meta_screen_get_display (screen);
  Display       *xdpy = mutter_plugin_get_xdisplay (plugin);
  gboolean       have_desktop = FALSE;
  gint           screen_width, screen_height;
  Window        *children, *l;
  guint          n_children;
  Window         root_win;
  Window         parent_win, root_win2;
  Status         status;
  Atom           desktop_atom;

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  /*
   * The do_init() method is called before the Manager starts managing
   * pre-existing windows, so we cannot query the windows via the normal
   * Mutter API, but have to use X lib to traverse the root window list.
   *
   * (We could probably add manage_all_windows() virtual to the plugin API
   * to split the initialization into two stages, but it is probably not worth
   * the hassle.)
   */
  root_win = RootWindow (xdpy, meta_screen_get_screen_number (screen));

  status = XQueryTree (xdpy, root_win, &root_win, &parent_win, &children,
                       &n_children);

  desktop_atom = meta_display_get_atom (display,
                                        META_ATOM__NET_WM_WINDOW_TYPE_DESKTOP);

  if (status)
    {
      guint i;
      Atom  type_atom;

      type_atom = meta_display_get_atom (display,
                                         META_ATOM__NET_WM_WINDOW_TYPE);

      for (l = children, i = 0; i < n_children; ++l, ++i)
        {
          unsigned long  n_items, ret_bytes;
          Atom           ret_type;
          int            ret_format;
          Atom          *type;

          XGetWindowProperty (xdpy, *l, type_atom, 0, 8192, False,
                              XA_ATOM, &ret_type, &ret_format,
                              &n_items, &ret_bytes, (unsigned char**)&type);

          if (type)
            {
              if (*type == desktop_atom)
                have_desktop = TRUE;

              XFree (type);
            }

          if (have_desktop)
            break;
        }

      XFree (children);
    }

  if (!have_desktop)
    {
      /*
       * Create a dummy desktop window.
       */
      Window               dwin;
      XSetWindowAttributes attr;

      attr.event_mask = ExposureMask;

      dwin = XCreateWindow (xdpy,
                            RootWindow (xdpy,
                                        meta_screen_get_screen_number (screen)),
                            0, 0, screen_width, screen_height, 0,
                            CopyFromParent, InputOnly, CopyFromParent,
                            CWEventMask, &attr);

      XChangeProperty (xdpy, dwin,
                       meta_display_get_atom (display,
                                              META_ATOM__NET_WM_WINDOW_TYPE),
                       XA_ATOM, 32, PropModeReplace,
                       (unsigned char *) &desktop_atom,
                       1);

      XMapWindow (xdpy, dwin);
    }

  /* FIXME: pull image from theme, css ? */
  priv->parallax_tex = clutter_texture_new_from_file
                        (PLUGIN_PKGDATADIR "/theme/panel/background-tile.png",
                         NULL);

  if (priv->parallax_tex == NULL)
    {
      g_warning ("Failed to load '"
                 PLUGIN_PKGDATADIR
                 "/theme/panel/background-tile.png', No tiled desktop image");
    }
  else
    {
      g_object_set (priv->parallax_tex,
                    "repeat-x", TRUE,
                    "repeat-y", TRUE,
                    NULL);
    }
}

/*
 * Core of the plugin init function, called for initial initialization and
 * by the reload() function. Returns TRUE on success.
 */
static gboolean
do_init (const char *params)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv = g_new0 (PluginPrivate, 1);

  guint          destroy_timeout           = DESTROY_TIMEOUT;
  guint          minimize_timeout          = MINIMIZE_TIMEOUT;
  guint          maximize_timeout          = MAXIMIZE_TIMEOUT;
  guint          map_timeout               = MAP_TIMEOUT;
  guint          switch_timeout            = SWITCH_TIMEOUT;
  guint          panel_slide_timeout       = PANEL_SLIDE_TIMEOUT;
  guint          ws_switcher_slide_timeout = WS_SWITCHER_SLIDE_TIMEOUT;

  const gchar   *name;
  ClutterActor  *overlay;
  ClutterActor  *panel;
  ClutterActor  *lowlight;
  gint           screen_width, screen_height;
  XRectangle     rect[2];
  XserverRegion  region;
  Display       *xdpy = mutter_plugin_get_xdisplay (plugin);
  ClutterColor   low_clr = { 0, 0, 0, 0x7f };
  GError        *err = NULL;

  gtk_init (NULL, NULL);

  /* tweak with env var as then possible to develop in desktop env. */
  if (!g_getenv("MUTTER_DISABLE_WS_CLAMP"))
    meta_prefs_set_num_workspaces (1);

  plugin->plugin_private = priv;

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             PLUGIN_PKGDATADIR "/theme/mutter-moblin.css",
                             &err);
  if (err)
    {
      g_warning (err->message);
      g_error_free (err);
    }

  name = plugin->name;
  plugin->name = _(name);
  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  rect[0].x = 0;
  rect[0].y = 0;
  rect[0].width = screen_width;
  rect[0].height = 1;

  rect[1].x = screen_width - WS_SWITCHER_SLIDE_THRESHOLD;
  rect[1].y = 0;
  rect[1].width = WS_SWITCHER_SLIDE_THRESHOLD;
  rect[1].height = screen_height;

  region = XFixesCreateRegion (xdpy, &rect[0], 2);

  priv->input_region = region;

  if (params)
    {
      if (strstr (params, "debug"))
        {
          g_debug ("%s: Entering debug mode.",
                   plugin->name);

          priv->debug_mode = TRUE;

          /*
           * Double the effect duration to make them easier to observe.
           */
          destroy_timeout           *= 2;
          minimize_timeout          *= 2;
          maximize_timeout          *= 2;
          map_timeout               *= 2;
          switch_timeout            *= 2;
          panel_slide_timeout       *= 2;
          ws_switcher_slide_timeout *= 2;
        }
    }

  priv->destroy_effect
    =  clutter_effect_template_new (clutter_timeline_new_for_duration (
							destroy_timeout),
                                    CLUTTER_ALPHA_SINE_INC);


  priv->minimize_effect
    =  clutter_effect_template_new (clutter_timeline_new_for_duration (
							minimize_timeout),
                                    CLUTTER_ALPHA_SINE_INC);

  priv->maximize_effect
    =  clutter_effect_template_new (clutter_timeline_new_for_duration (
							maximize_timeout),
                                    CLUTTER_ALPHA_SINE_INC);

  priv->map_effect
    =  clutter_effect_template_new (clutter_timeline_new_for_duration (
							map_timeout),
                                    CLUTTER_ALPHA_SINE_INC);

  priv->switch_workspace_effect
    =  clutter_effect_template_new (clutter_timeline_new_for_duration (
							switch_timeout),
                                    CLUTTER_ALPHA_SINE_INC);

  /* better syncing as multiple groups run off this */
  clutter_effect_template_set_timeline_clone (priv->switch_workspace_effect,
                                              TRUE);

  priv->switch_workspace_arrow_effect
    =  clutter_effect_template_new (clutter_timeline_new_for_duration (
							switch_timeout*4),
                                    CLUTTER_ALPHA_SINE_INC);

  overlay = mutter_plugin_get_overlay_group (plugin);

  lowlight = clutter_rectangle_new_with_color (&low_clr);
  priv->lowlight = lowlight;
  clutter_actor_set_size (lowlight, screen_width, screen_height);

  /*
   * This also creates the launcher.
   */
  panel = priv->panel = make_panel (screen_width);
  clutter_actor_realize (priv->panel_shadow);
  clutter_actor_set_y (panel, -clutter_actor_get_height (priv->panel_shadow));

  clutter_container_add (CLUTTER_CONTAINER (overlay), lowlight, panel, NULL);

  clutter_actor_hide (lowlight);

  priv->panel_slide_effect
    =  clutter_effect_template_new (clutter_timeline_new_for_duration (
							panel_slide_timeout),
                                    CLUTTER_ALPHA_SINE_INC);

  priv->ws_switcher_slide_effect
    =  clutter_effect_template_new (clutter_timeline_new_for_duration (
						ws_switcher_slide_timeout),
                                                CLUTTER_ALPHA_SINE_INC);

  /*
   * Set up the stage even processing
   */
  disable_stage (plugin, CurrentTime);

  /*
   * Hook to the captured signal, so we get to see all events before our
   * children and do not interfere with their event processing.
   */
  g_signal_connect (mutter_plugin_get_stage (plugin),
                    "captured-event", G_CALLBACK (stage_input_cb),
                    GINT_TO_POINTER (TRUE));

  g_signal_connect (mutter_plugin_get_stage (plugin),
                    "button-press-event", G_CALLBACK (stage_input_cb),
                    GINT_TO_POINTER (FALSE));

  clutter_set_motion_events_enabled (TRUE);

  setup_parallax_effect ();

  setup_startup_notification ();

  return TRUE;
}

static void
free_plugin_private (PluginPrivate *priv)
{
  MutterPlugin *plugin;
  Display      *xdpy;

  if (!priv)
    return;

  xdpy = mutter_plugin_get_xdisplay (plugin);

  if (priv->input_region)
    XFixesDestroyRegion (xdpy, priv->input_region);

  g_object_unref (priv->destroy_effect);
  g_object_unref (priv->minimize_effect);
  g_object_unref (priv->maximize_effect);
  g_object_unref (priv->switch_workspace_effect);
  g_object_unref (priv->switch_workspace_arrow_effect);

  g_free (priv);

  mutter_get_plugin()->plugin_private = NULL;
}

/*
 * Called by the plugin manager when we stuff like the command line parameters
 * changed.
 */
static gboolean
reload (const char *params)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  if (do_init (params))
    {
      /* Success; free the old private struct */
      free_plugin_private (priv);
      return TRUE;
    }
  else
    {
      /* Fail -- fall back to the old private. */
      plugin->plugin_private = priv;
    }

  return FALSE;
}

/*
 * GModule unload function -- do any cleanup required.
 */
G_MODULE_EXPORT void g_module_unload (GModule *module);
G_MODULE_EXPORT void g_module_unload (GModule *module)
{
  PluginPrivate *priv = mutter_get_plugin()->plugin_private;

  free_plugin_private (priv);
}
