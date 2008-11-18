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
#include "mutter-plugin.h"

#include <libintl.h>
#define _(x) dgettext (GETTEXT_PACKAGE, x)
#define N_(x) x

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <nbtk/nbtk.h>
#include <gmodule.h>
#include <string.h>

#include "nutter/nutter-grid.h"
#include "nutter/nutter-ws-icon.h"
#include "nutter/nutter-scale-group.h"
#include "compositor-mutter.h"

#define DESTROY_TIMEOUT             250
#define MINIMIZE_TIMEOUT            250
#define MAXIMIZE_TIMEOUT            250
#define MAP_TIMEOUT                 250
#define SWITCH_TIMEOUT              500
#define PANEL_SLIDE_TIMEOUT         250
#define PANEL_SLIDE_THRESHOLD       2
#define PANEL_HEIGHT                40
#define WS_SWITCHER_SLIDE_TIMEOUT   250
#define WS_SWITCHER_SLIDE_THRESHOLD 3
#define ACTOR_DATA_KEY "MCCP-moblin-netbook-actor-data"

#define APP_SWITCHER_CELL_WIDTH  200
#define APP_SWITCHER_CELL_HEIGHT 200

#define WORKSPACE_CELL_WIDTH  100
#define WORKSPACE_CELL_HEIGHT 80

#define WORKSPACE_BORDER 20

#define WORKSPACE_CHOOSER_TIMEOUT 3000
#define WORKSPACE_CHOOSER_BORDER_WIDTH 1

#define MAX_WORKSPACES 8

static GQuark actor_data_quark = 0;

typedef struct PluginPrivate PluginPrivate;
typedef struct ActorPrivate  ActorPrivate;

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

static void hide_panel (void);

static gint _desktop_paint_offset = 0;
static ClutterActor* _desktop_tex = NULL;

/*
 * Create the plugin struct; function pointers initialized in
 * g_module_check_init().
 */
MUTTER_DECLARE_PLUGIN ();

/*
 * Plugin private data that we store in the .plugin_private member.
 */
struct PluginPrivate
{
  ClutterEffectTemplate *destroy_effect;
  ClutterEffectTemplate *minimize_effect;
  ClutterEffectTemplate *maximize_effect;
  ClutterEffectTemplate *map_effect;
  ClutterEffectTemplate *switch_workspace_effect;
  ClutterEffectTemplate *switch_workspace_arrow_effect;
  ClutterEffectTemplate *panel_slide_effect;
  ClutterEffectTemplate *ws_switcher_slide_effect;

  /* Valid only when switch_workspace effect is in progress */
  ClutterTimeline       *tml_switch_workspace0;
  ClutterTimeline       *tml_switch_workspace1;
  GList                **actors;
  ClutterActor          *desktop1;
  ClutterActor          *desktop2;

  ClutterActor          *d_overlay ; /* arrow indicator */
  ClutterActor          *panel;

  ClutterActor          *switcher;
  ClutterActor          *workspace_switcher;
  ClutterActor          *workspace_chooser;
  ClutterActor          *lowlight;

  XserverRegion          input_region;

  gint                   next_app_workspace;
  gchar                 *app_to_start;

  gboolean               debug_mode : 1;
  gboolean               panel_out  : 1;
  gboolean               panel_out_in_progress : 1;
  gboolean               panel_back_in_progress : 1;

  guint                  workspace_chooser_timeout;
};

/*
 * Per actor private data we attach to each actor.
 */
struct ActorPrivate
{
  ClutterActor *orig_parent;
  gint          orig_x;
  gint          orig_y;

  ClutterTimeline *tml_minimize;
  ClutterTimeline *tml_maximize;
  ClutterTimeline *tml_destroy;
  ClutterTimeline *tml_map;

  gboolean      is_minimized : 1;
  gboolean      is_maximized : 1;
};

/*
 * Actor private data accessor
 */
static void
free_actor_private (gpointer data)
{
  if (G_LIKELY (data != NULL))
    g_slice_free (ActorPrivate, data);
}

static ActorPrivate *
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
  ClutterCloneTexturePrivate  *priv;
  ClutterActor                *parent_texture;
  gint                         x_1, y_1, x_2, y_2;
  ClutterColor                 col = { 0xff, 0xff, 0xff, 0xff };
  CoglHandle                   cogl_texture;
  ClutterFixed                 t_w, t_h;
  guint                        tex_width, tex_height;
  guint w, h;

  clutter_actor_get_size (_desktop_tex, &w, &h);

  cogl_translate (_desktop_paint_offset - w/4 , 0 , 0);

  cogl_texture 
       = clutter_texture_get_cogl_texture (CLUTTER_TEXTURE(_desktop_tex));

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

  mutter_plugin_effect_completed (plugin, actor_for_cb,
                                  MUTTER_PLUGIN_SWITCH_WORKSPACE);
}

static void
on_workspace_frame_change (ClutterTimeline *timeline,
                           gint             frame_num,
                           gpointer         data)
{
  _desktop_paint_offset += ((gint)data) * -1;
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
  clutter_actor_set_position (workspace_slider1, to_x, to_y);
  clutter_actor_raise_top (workspace_slider1);

  /* center arrow */
  clutter_actor_get_size (indicator_group, &indicator_width, &indicator_height);
  clutter_actor_set_position (indicator_group,
			      (screen_width - indicator_width) / 2,
			      (screen_height - indicator_height) / 2);

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

static void
on_map_effect_complete (ClutterActor *actor, gpointer data)
{
  /*
   * Must reverse the effect of the effect.
   */
  MutterPlugin *plugin = mutter_get_plugin ();
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
      && _desktop_tex != NULL ) /* FIXME */
    {
      gint screen_width, screen_height;

      mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

      clutter_actor_set_size (_desktop_tex, 
                              screen_width * 8,
                              screen_height);

      clutter_actor_set_parent (_desktop_tex, actor);

      g_signal_connect (actor,
                        "paint", G_CALLBACK (on_desktop_pre_paint),
                        NULL);

      mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
      return;
    }

  if (type == META_COMP_WINDOW_NORMAL)
    {
      ActorPrivate *apriv = get_actor_private (mcw);

      /*
       * Put the window on the requested wokspace, if any.
       *
       * NB: this is for prototyping only; if this functionality should remain
       *     it probably needs to use start up notification.
       *
       * What we do is:
       *
       * 1. Dispatch _NET_WM_DESKTOP client message; when this is processed by
       *    metacity, it moves the window on the requested desktop.
       *
       * 2. Activate the desktop -- this results in switch from current to
       *    whatever desktop.
       */
      if (priv->next_app_workspace > -2)
        {
          static Atom net_wm_desktop = 0;
          MetaWindow  *mw = mutter_window_get_meta_window (mcw);
          MetaScreen  *screen = meta_window_get_screen (mw);
          MetaDisplay *display = meta_screen_get_display (screen);
          Display     *xdpy = meta_display_get_xdisplay (display);

          if (!net_wm_desktop)
            net_wm_desktop = XInternAtom (xdpy, "_NET_WM_DESKTOP", False);

          if (mw)
            {
              Window xwin = meta_window_get_xwindow (mw);
              XEvent ev;

              memset(&ev, 0, sizeof(ev));

              ev.xclient.type = ClientMessage;
              ev.xclient.window = xwin;
              ev.xclient.message_type = net_wm_desktop;
              ev.xclient.format = 32;
              ev.xclient.data.l[0] = priv->next_app_workspace;

              /*
               * Metacity watches for property changes, hence the
               * PropertyChangeMask
               */
              XSendEvent(xdpy, xwin, False, PropertyChangeMask, &ev);
              XSync(xdpy, False);

              if (priv->next_app_workspace > -1)
                {
                  GList * l;
                  MetaWorkspace * workspace;

                  workspace =
                    meta_screen_get_workspace_by_index (screen,
                                                    priv->next_app_workspace);

                  if (workspace)
                    meta_workspace_activate_with_focus (workspace, mw,
                                                        CurrentTime);
                }
            }

          priv->next_app_workspace = -2;
        }

      clutter_actor_move_anchor_point_from_gravity (actor,
                                                    CLUTTER_GRAVITY_CENTER);

      clutter_actor_set_scale (actor, 0.0, 0.0);
      clutter_actor_show (actor);

      apriv->tml_map = clutter_effect_scale (priv->map_effect,
                                             actor,
                                             1.0,
                                             1.0,
                                             (ClutterEffectCompleteFunc)
                                             on_map_effect_complete,
                                             NULL);

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

/*
 * Simple TV-out like effect.
 */
static void
destroy (MutterWindow *mcw)
{
  MutterPlugin       *plugin = mutter_get_plugin ();
  PluginPrivate      *priv   = plugin->plugin_private;
  MetaCompWindowType  type;
  ClutterActor       *actor  = CLUTTER_ACTOR (mcw);

  type = mutter_window_get_window_type (mcw);

  if (type == META_COMP_WINDOW_NORMAL)
    {
      ActorPrivate *apriv  = get_actor_private (mcw);

      clutter_actor_move_anchor_point_from_gravity (actor,
                                                    CLUTTER_GRAVITY_CENTER);

      apriv->tml_destroy = clutter_effect_scale (priv->destroy_effect,
                                                 actor,
                                                 1.0,
                                                 0.0,
                                                 (ClutterEffectCompleteFunc)
                                                 on_destroy_effect_complete,
                                                 NULL);
    }
  else
    mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_DESTROY);
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
 * Use this function to disable stage input
 *
 * Used by the completion callback for the panel in/out effects
 */
static void
disable_stage (MutterPlugin *plugin)
{
  PluginPrivate *priv = plugin->plugin_private;

  mutter_plugin_set_stage_input_region (plugin, priv->input_region);
}

static void
on_panel_effect_complete (ClutterActor *panel, gpointer data)
{
  gboolean       reactive = GPOINTER_TO_INT (data);
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;

  if (reactive)
    {
      priv->panel_out_in_progress = FALSE;
      mutter_plugin_set_stage_reactive (plugin, reactive);
    }
  else
    {
      priv->panel_back_in_progress = FALSE;

      if (!priv->workspace_chooser && !priv->workspace_switcher)
        disable_stage (plugin);
    }
}

static gboolean
xevent_filter (XEvent *xev)
{
  MutterPlugin *plugin = mutter_get_plugin ();
  ClutterActor                *stage;

  stage = mutter_plugin_get_stage (plugin);

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
      on_map_effect_complete (actor, NULL);
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

/*
 * Clone lifecycle house keeping.
 *
 * The workspace switcher and chooser hold clones of the window glx textures.
 * We need to ensure that when the master texture disappears, we remove the
 * clone as well. We do this via weak reference on the original object, and
 * so when the clone itself disappears, we need to remove the weak reference
 * from the master.
 */
static void switcher_clone_weak_notify (gpointer data, GObject *object);

static void
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

static void
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

static void
hide_app_switcher (void)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  if (!priv->switcher)
    return;

  clutter_actor_destroy (priv->switcher);

  disable_stage (plugin);

  priv->switcher = NULL;
}

/*
 * Handles click on an application thumb in application switcher by
 * activating the corresponding application.
 */
static gboolean
app_switcher_clone_input_cb (ClutterActor *clone,
                             ClutterEvent *event,
                             gpointer      data)
{
  MutterWindow  *mw = data;
  MetaWindow    *window;
  MetaWorkspace *workspace;

  window    = mutter_window_get_meta_window (mw);
  workspace = meta_window_get_workspace (window);

  hide_app_switcher ();
  meta_workspace_activate_with_focus (workspace, window, event->any.time);

  return FALSE;
}

/*
 * Workspace switcher, used to switch between existing workspaces.
 */
static void
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
static ClutterActor *
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
workspace_input_cb (ClutterActor *clone,
                    ClutterEvent *event,
                    gpointer      data)
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

      n_windows[ws_indx]++;
      nbtk_table_add_actor (NBTK_TABLE (table), clone, n_windows[ws_indx], ws_indx);
      ws_max_windows = MAX (ws_max_windows, n_windows[ws_indx]);
    }
  g_free (n_windows);


  clutter_actor_set_size (CLUTTER_ACTOR (table),
                          WORKSPACE_CELL_WIDTH * ws_count,
                          WORKSPACE_CELL_HEIGHT * (ws_max_windows + 1));

  return CLUTTER_ACTOR (table);
}

/*
 * Creates a grid of workspaces, which contains a header row with
 * workspace symbolic icon, and the workspaces below it.
 *
 * It can be expanded (WS chooser) or unexpanded (WS switcher).
 */
static ClutterActor *
make_workspace_grid (GCallback  ws_callback,
                     gint      *n_workspaces)
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;
  GList         *l;
  NutterGrid    *grid;
  ClutterActor  *grid_actor;
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

  grid_actor = nutter_grid_new ();
  grid = NUTTER_GRID (grid_actor);

  nutter_grid_set_column_major (grid, FALSE);
  nutter_grid_set_row_gap (grid, CLUTTER_UNITS_FROM_INT (5));
  nutter_grid_set_column_gap (grid, CLUTTER_UNITS_FROM_INT (5));
  nutter_grid_set_max_size (grid, screen_width, screen_height);
  nutter_grid_set_max_dimension (grid, active_ws);
  nutter_grid_set_homogenous_columns (grid, TRUE);
  nutter_grid_set_halign (grid, 0.5);

  for (i = 0; i < active_ws; ++i)
    {
      gchar *s = g_strdup_printf ("%d", i + 1);

      ws_label = make_workspace_label (s);

      g_signal_connect (ws_label, "button-press-event",
                        ws_callback, GINT_TO_POINTER (i));

      clutter_actor_set_reactive (ws_label, TRUE);

      clutter_container_add_actor (CLUTTER_CONTAINER (grid), ws_label);

      g_free (s);
    }

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

      /*
       * Scale unexpanded workspaces to fit the predefined size of the grid cell
       */
      clutter_actor_set_scale (ws, ws_scale_x, ws_scale_y);

      g_signal_connect (ws, "button-press-event",
                        ws_callback, GINT_TO_POINTER (ws_count));

      clutter_actor_set_reactive (ws, TRUE);

      clutter_container_add_actor (CLUTTER_CONTAINER (grid), ws);

      ++ws_count;
      l = l->next;
    }


  if (n_workspaces)
    *n_workspaces = ws_count;

  return grid_actor;
}

/*
 * Constructs and shows the workspace switcher actor.
 */
static void
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
 * Helper method to spawn and application. Will eventually be replaced
 * by libgnome-menu, or something like that.
 */
static void
spawn_app (const gchar *path)
{
  gchar *argv[2] = {NULL, NULL};

  argv[0] = g_strdup (path);

  if (!path)
    return;

  if (!g_spawn_async (NULL, &argv[0], NULL, G_SPAWN_SEARCH_PATH,
                      NULL, NULL, NULL, NULL))
    {
      MutterPlugin  *plugin  = mutter_get_plugin ();
      PluginPrivate *priv    = plugin->plugin_private;

      g_free (priv->app_to_start);
      priv->app_to_start = NULL;
      priv->next_app_workspace = -2;
    }
}

/*
 * Workspace chooser
 *
 * The chooser is shown when user launches an application from the panel; it
 * allows the user to select an existing workspace to place the application on,
 * or, by default, it creates a new workspace for the application.
 */
static void
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
 * Handles button press on a workspace withing the chooser by placing the
 * starting application on the given workspace.
 */
static gboolean
workspace_chooser_input_cb (ClutterActor *clone,
                            ClutterEvent *event,
                            gpointer      data)
{
  gint           indx   = GPOINTER_TO_INT (data);
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;
  MetaScreen    *screen = mutter_plugin_get_screen (plugin);
  MetaWorkspace *workspace;

  workspace = meta_screen_get_workspace_by_index (screen, indx);

  if (!workspace)
    {
      g_warning ("No workspace specified, %s:%d\n", __FILE__, __LINE__);
      return FALSE;
    }

  priv->next_app_workspace = indx;

  hide_workspace_chooser ();

  spawn_app (priv->app_to_start);

  g_free (priv->app_to_start);
  priv->app_to_start = NULL;

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
  gint           ws_count  = GPOINTER_TO_INT (data);

  hide_workspace_chooser ();

  /*
   * Workspaces are restricted to MAX_WORKSPACES
   */
  if (ws_count < MAX_WORKSPACES)
    {
      priv->next_app_workspace = ws_count;
      meta_screen_append_new_workspace (screen, TRUE, event->any.time);
    }
  else
    {
      priv->next_app_workspace = -2;
    }

  spawn_app (priv->app_to_start);

  g_free (priv->app_to_start);
  priv->app_to_start = NULL;

  return FALSE;
}

/*
 * Triggers when the user does not interact with the chooser; it places the
 * starting application on a new workspace.
 */
static gboolean
workspace_chooser_timeout_cb (gpointer data)
{
  MutterPlugin  *plugin    = mutter_get_plugin ();
  PluginPrivate *priv      = plugin->plugin_private;
  MetaScreen    *screen    = mutter_plugin_get_screen (plugin);
  gint           ws_count  = GPOINTER_TO_INT (data);

  hide_workspace_chooser ();

  /*
   * Workspaces are restricted to MAX_WORKSPACES
   */
  if (ws_count < MAX_WORKSPACES)
    {
      priv->next_app_workspace = ws_count;
      meta_screen_append_new_workspace (screen, TRUE, CurrentTime);
    }
  else
    {
      priv->next_app_workspace = -2;
    }

  spawn_app (priv->app_to_start);

  g_free (priv->app_to_start);
  priv->app_to_start = NULL;
  priv->next_app_workspace = -2;

  /* One off */
  return FALSE;
}

/*
 * Creates and shows the workspace chooser.
 */
static void
show_workspace_chooser (const gchar *app_path)
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

  g_free (priv->app_to_start);
  priv->app_to_start = g_strdup (app_path);

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

  grid = make_workspace_grid (G_CALLBACK (workspace_chooser_input_cb),
                              &ws_count);
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

  g_signal_connect (new_ws, "button-press-event",
                    G_CALLBACK (new_workspace_input_cb),
                    GINT_TO_POINTER (ws_count));

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

  priv->workspace_chooser_timeout =
    g_timeout_add (WORKSPACE_CHOOSER_TIMEOUT, workspace_chooser_timeout_cb,
                   GINT_TO_POINTER (ws_count));
}

/*
 * The slide-out top panel.
 */
static void
hide_panel ()
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;
  guint          height = clutter_actor_get_height (priv->panel);
  gint           x      = clutter_actor_get_x (priv->panel);

  priv->panel_back_in_progress  = TRUE;

  clutter_effect_move (priv->panel_slide_effect,
                       priv->panel, x, -height,
                       on_panel_effect_complete,
                       GINT_TO_POINTER (FALSE));
  priv->panel_out = FALSE;
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

      if (!capture &&
          priv->workspace_switcher && event->type == CLUTTER_BUTTON_PRESS)
        {
          hide_workspace_switcher ();
        }

      if (priv->panel_out &&
          (event->type == CLUTTER_BUTTON_PRESS ||
           (!priv->switcher && !priv->workspace_switcher)))
        {
          guint height = clutter_actor_get_height (priv->panel);

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
                               on_panel_effect_complete,
                               GINT_TO_POINTER (TRUE));

          priv->panel_out = TRUE;
        }
    }
  else if (event->type == CLUTTER_KEY_RELEASE)
    {
      ClutterKeyEvent *kev = (ClutterKeyEvent *) event;

      g_print ("*** key press event (key:%c) ***\n",
	       clutter_key_event_symbol (kev));

    }

  return FALSE;
}

/*
 * Panel buttons.
 */
static gboolean
app_launcher_input_cb (ClutterActor *actor,
                       ClutterEvent *event,
                       gpointer      data)
{
  show_workspace_chooser (data);
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

static ClutterActor *
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

/*
 * Core of the plugin init function, called for initial initialization and
 * by the reload() function. Returns TRUE on success.
 */
static gboolean
do_init (const char *params)
{
  MutterPlugin *plugin = mutter_get_plugin ();

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

  plugin->plugin_private = priv;

  priv->next_app_workspace = -2;

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

  panel = priv->panel = make_panel (screen_width);
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

  clutter_actor_set_position (panel, 0,
                              -clutter_actor_get_height (panel));

  /*
   * Set up the stage even processing
   */
  disable_stage (plugin);

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


  _desktop_tex = clutter_texture_new_from_file ("/tmp/bck.jpg", NULL);

  if (_desktop_tex == NULL)
    {
      g_warning ("Failed to load /tmp/bck.jpg, No tiled desktop image");
    }
  else
    {
      g_object_set (_desktop_tex,
                    "repeat-x", TRUE,
                    "repeat-y", TRUE,
                    NULL);
    }

  clutter_set_motion_events_enabled (TRUE);

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
