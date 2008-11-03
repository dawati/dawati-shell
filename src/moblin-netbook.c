/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
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
#include <gmodule.h>
#include <string.h>

#include "tidy/tidy-grid.h"

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
#define WORKSPACE_DATA_KEY "MCCP-moblin-netbook-workspace-data"

#define SWITCHER_CELL_WIDTH  200
#define SWITCHER_CELL_HEIGHT 200

#define WORKSPACE_CELL_WIDTH  100
#define WORKSPACE_CELL_HEIGHT 80

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

static gboolean switcher_clone_input_cb (ClutterActor *clone,
                                         ClutterEvent *event,
                                         gpointer      data);

static void show_workspace_chooser (const gchar *app_path);

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
  ClutterTimeline       *tml_switch_workspace1;
  ClutterTimeline       *tml_switch_workspace2;
  GList                **actors;
  ClutterActor          *desktop1;
  ClutterActor          *desktop2;

  ClutterActor          *d_overlay ; /* arrow indicator */
  ClutterActor          *panel;

  ClutterActor          *switcher;
  ClutterActor          *workspace_switcher;
  ClutterActor          *workspace_chooser;

  XserverRegion          input_region;

  gint                   next_app_workspace;
  gchar                 *app_to_start;

  gboolean               debug_mode : 1;
  gboolean               panel_out  : 1;
  gboolean               panel_out_in_progress : 1;
  gboolean               panel_back_in_progress : 1;
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
  ppriv->tml_switch_workspace1 = NULL;
  ppriv->tml_switch_workspace2 = NULL;
  ppriv->desktop1 = NULL;
  ppriv->desktop2 = NULL;

  mutter_plugin_effect_completed (plugin, actor_for_cb,
                                  MUTTER_PLUGIN_SWITCH_WORKSPACE);
}

static void
switch_workspace (const GList **actors, gint from, gint to,
                  MetaMotionDirection direction)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *ppriv  = plugin->plugin_private;
  GList         *l;
  gint           n_workspaces;
  ClutterActor  *group1  = clutter_group_new ();
  ClutterActor  *group2  = clutter_group_new ();
  ClutterActor  *group3  = clutter_group_new ();
  ClutterActor  *stage, *label, *rect, *window_layer, *overlay_layer;
  gint           to_x, to_y, from_x = 0, from_y = 0;
  ClutterColor   white = { 0xff, 0xff, 0xff, 0xff };
  ClutterColor   black = { 0x33, 0x33, 0x33, 0xff };
  gint           screen_width;
  gint           screen_height;

  stage = mutter_plugin_get_stage (plugin);

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  window_layer = mutter_plugin_get_window_group (plugin);
  overlay_layer = mutter_plugin_get_overlay_group (plugin);

  clutter_container_add_actor (CLUTTER_CONTAINER (window_layer), group1);
  clutter_container_add_actor (CLUTTER_CONTAINER (window_layer), group2);
  clutter_container_add_actor (CLUTTER_CONTAINER (overlay_layer), group3);

  if (from == to)
    {
      clutter_actor_destroy (group3);
      clutter_actor_destroy (group2);
      clutter_actor_destroy (group1);

      mutter_plugin_effect_completed (plugin, NULL,
                                      MUTTER_PLUGIN_SWITCH_WORKSPACE);
      return;
    }

  n_workspaces = g_list_length (plugin->work_areas);

  l = g_list_last (*((GList**) actors));

  while (l)
    {
      MutterWindow *mcw  = l->data;
      ActorPrivate *priv = get_actor_private (mcw);
      ClutterActor *a    = CLUTTER_ACTOR (mcw);
      gint          workspace;

      workspace = mutter_window_get_workspace (mcw);

      if (workspace == to || workspace == from)
        {
          gint x, y;
          guint w, h;

          clutter_actor_get_position (a, &x, &y);
          clutter_actor_get_size (a, &w, &h);

          priv->orig_parent = clutter_actor_get_parent (a);

          clutter_actor_reparent (a, workspace == to ? group2 : group1);
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

      l = l->prev;
    }

  /* Make arrow indicator */
  rect = clutter_rectangle_new ();
  clutter_rectangle_set_color (CLUTTER_RECTANGLE (rect), &white);
  clutter_container_add_actor (CLUTTER_CONTAINER (group3), rect);

  label = clutter_label_new ();
  clutter_label_set_font_name (CLUTTER_LABEL (label), "Sans Bold 148");
  clutter_label_set_color (CLUTTER_LABEL (label), &black);
  clutter_container_add_actor (CLUTTER_CONTAINER (group3), label);

  clutter_actor_set_size (rect,
                          clutter_actor_get_width (label),
                          clutter_actor_get_height (label));

  ppriv->actors  = (GList **)actors;
  ppriv->desktop1 = group1;
  ppriv->desktop2 = group2;
  ppriv->d_overlay = group3;

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
  clutter_actor_set_position (group2, to_x, to_y); /* *-1 for simpler */
  clutter_actor_raise_top (group2);

  /* center arrow */
  clutter_actor_set_position
                    (group3,
                     (screen_width - clutter_actor_get_width (group3)) / 2,
                     (screen_height - clutter_actor_get_height (group3)) / 2);


  /* workspace were going too */
  ppriv->tml_switch_workspace2 =
    clutter_effect_move (ppriv->switch_workspace_effect, group2,
                         0, 0,
                         on_switch_workspace_effect_complete,

                         actors);
  /* coming from */
  ppriv->tml_switch_workspace1 =
    clutter_effect_move (ppriv->switch_workspace_effect, group1,
                         to_x, to_y,
                         NULL, NULL);

  /* arrow */
  clutter_effect_fade (ppriv->switch_workspace_arrow_effect, group3,
                       0,
                       NULL, NULL);


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

                  l = meta_screen_get_workspaces (screen);
                  workspace = g_list_nth_data (l, priv->next_app_workspace);

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

      if (!priv->workspace_chooser)
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

      if (ppriv->tml_switch_workspace1)
        {
          clutter_timeline_stop (ppriv->tml_switch_workspace1);
          clutter_timeline_stop (ppriv->tml_switch_workspace2);
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

/*
 * This is a simple example of how a switcher might access the windows.
 *
 * Note that we use ClutterCloneTexture hooked up to the texture *inside*
 * MutterWindow (with FBO support, we could clone the entire MutterWindow,
 * although for the switcher purposes that is probably not what is wanted
 * anyway).
 */
static void
hide_switcher (void)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  if (!priv->switcher)
    return;

  clutter_actor_destroy (priv->switcher);

  disable_stage (plugin);

  priv->switcher = NULL;
}

static void
show_switcher (void)
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;
  ClutterActor  *overlay;
  GList         *l;
  ClutterActor  *switcher;
  TidyGrid      *grid;
  guint          panel_height;
  gint           panel_y;
  gint           screen_width, screen_height;

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  switcher = tidy_grid_new ();

  grid = TIDY_GRID (switcher);

  tidy_grid_set_homogenous_rows (grid, TRUE);
  tidy_grid_set_homogenous_columns (grid, TRUE);
  tidy_grid_set_column_major (grid, FALSE);
  tidy_grid_set_row_gap (grid, CLUTTER_UNITS_FROM_INT (10));
  tidy_grid_set_column_gap (grid, CLUTTER_UNITS_FROM_INT (10));

  l = mutter_plugin_get_windows (plugin);
  while (l)
    {
      MutterWindow       *mw   = l->data;
      MetaCompWindowType  type = mutter_window_get_window_type (mw);
      ClutterActor       *a    = CLUTTER_ACTOR (mw);
      ClutterActor       *texture;
      ClutterActor       *clone;
      guint               w, h;
      gdouble             s_x, s_y, s;

      /*
       * Only show regular windows.
       */
      if (mutter_window_is_override_redirect (mw) ||
          type != META_COMP_WINDOW_NORMAL)
        {
          l = l->next;
          continue;
        }

      texture = mutter_window_get_texture (mw);
      clone   = clutter_clone_texture_new (CLUTTER_TEXTURE (texture));

      g_signal_connect (clone,
                        "button-press-event",
                        G_CALLBACK (switcher_clone_input_cb), mw);

      g_object_weak_ref (G_OBJECT (mw), switcher_origin_weak_notify, clone);
      g_object_weak_ref (G_OBJECT (clone), switcher_clone_weak_notify, mw);

      /*
       * Scale clone to fit the predefined size of the grid cell
       */
      clutter_actor_get_size (a, &w, &h);
      s_x = (gdouble) SWITCHER_CELL_WIDTH  / (gdouble) w;
      s_y = (gdouble) SWITCHER_CELL_HEIGHT / (gdouble) h;

      s = s_x < s_y ? s_x : s_y;

      if (s_x < s_y)
        clutter_actor_set_size (clone,
                                (guint)((gdouble)w * s_x),
                                (guint)((gdouble)h * s_x));
      else
        clutter_actor_set_size (clone,
                                (guint)((gdouble)w * s_y),
                                (guint)((gdouble)h * s_y));

      clutter_actor_set_reactive (clone, TRUE);

      clutter_container_add_actor (CLUTTER_CONTAINER (grid), clone);
      l = l->next;
    }

  if (priv->switcher)
    hide_switcher ();

  priv->switcher = switcher;

  panel_height = clutter_actor_get_height (priv->panel);
  panel_y      = clutter_actor_get_y (priv->panel);

  clutter_actor_set_position (switcher, 10, panel_height + panel_y);

  overlay = mutter_plugin_get_overlay_group (plugin);
  clutter_container_add_actor (CLUTTER_CONTAINER (overlay), switcher);

  clutter_actor_set_width (switcher, screen_width);

  mutter_plugin_set_stage_reactive (plugin, TRUE);
}

static void
toggle_switcher ()
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;

  if (priv->switcher)
    hide_switcher ();
  else
    show_switcher ();
}

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

static ClutterActor *
ensure_nth_workspace (GList **list, gint n)
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
      ClutterActor *group = clutter_group_new ();
      ClutterColor  background_clr = { 0, 0, 0, 0};
      ClutterActor *background;

      /*
       * We need to add background, otherwise if the ws is empty, the group
       * will have size 0x0, and not respond to clicks.
       */
      background =  clutter_rectangle_new_with_color (&background_clr);
      clutter_actor_set_size (background, screen_width, screen_height);
      clutter_container_add_actor (CLUTTER_CONTAINER (group), background);

      tmp = g_list_append (tmp, group);

      ++i;
    }

  g_assert (tmp);

  *list = g_list_concat (*list, tmp);

  return g_list_last (*list)->data;
}

static gboolean
workspace_input_cb (ClutterActor *clone,
                    ClutterEvent *event,
                    gpointer      data)
{
  MetaWorkspace *workspace = data;

  if (!workspace)
    {
      g_warning ("No workspace specified, %s:%d\n", __FILE__, __LINE__);
      return FALSE;
    }

  hide_workspace_switcher ();
  meta_workspace_activate (workspace, event->any.time);

  return FALSE;
}

static ClutterActor *
make_workspace_grid (GCallback ws_callback, gint *n_workspaces)
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;
  GList         *l;
  TidyGrid      *grid;
  ClutterActor  *grid_actor;
  gint           screen_width, screen_height;
  GList         *workspaces = NULL;
  gdouble        ws_scale_x, ws_scale_y;
  gint           ws_count = 0;
  MetaScreen    *screen = mutter_plugin_get_screen (plugin);

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  ws_scale_x = (gdouble) WORKSPACE_CELL_WIDTH  / (gdouble) screen_width;
  ws_scale_y = (gdouble) WORKSPACE_CELL_HEIGHT / (gdouble) screen_height;

  grid_actor = tidy_grid_new ();
  grid = TIDY_GRID (grid_actor);

  tidy_grid_set_homogenous_rows (grid, TRUE);
  tidy_grid_set_homogenous_columns (grid, TRUE);
  tidy_grid_set_column_major (grid, FALSE);
  tidy_grid_set_row_gap (grid, CLUTTER_UNITS_FROM_INT (10));
  tidy_grid_set_column_gap (grid, CLUTTER_UNITS_FROM_INT (10));

  l = mutter_plugin_get_windows (plugin);
  l = g_list_last (l);

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
          l = l->prev;
          continue;
        }


      workspace = ensure_nth_workspace (&workspaces, ws_indx);

      g_assert (workspace);

      texture = mutter_window_get_texture (mw);
      clone   = clutter_clone_texture_new (CLUTTER_TEXTURE (texture));

      clutter_actor_get_position (CLUTTER_ACTOR (mw), &x, &y);
      clutter_actor_set_position (clone, x, y);
      clutter_actor_set_reactive (clone, FALSE);

      g_object_weak_ref (G_OBJECT (mw), switcher_origin_weak_notify, clone);
      g_object_weak_ref (G_OBJECT (clone), switcher_clone_weak_notify, mw);

      clutter_container_add_actor (CLUTTER_CONTAINER (workspace), clone);

      l = l->prev;
    }

  l = workspaces;
  while (l)
    {
      ClutterActor  *ws = l->data;
      MetaWorkspace *meta_ws;

      meta_ws = meta_screen_get_workspace_by_index (screen, ws_count);

      /*
       * Scale workspace to fit the predefined size of the grid cell
       */
      clutter_actor_set_scale (ws, ws_scale_x, ws_scale_y);

      g_signal_connect (ws, "button-press-event", ws_callback, meta_ws);

      clutter_actor_set_reactive (ws, TRUE);

      clutter_container_add_actor (CLUTTER_CONTAINER (grid), ws);

      ++ws_count;
      l = l->next;
    }

  /*
   * TODO -- fix TidyGrid, so we do not have to set the width explicitely.
   */
  clutter_actor_set_size (grid_actor,
                          ws_count * WORKSPACE_CELL_WIDTH,
                          WORKSPACE_CELL_HEIGHT);

  if (n_workspaces)
    *n_workspaces = ws_count;

  return grid_actor;
}

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
  ClutterColor   background_clr = { 0x44, 0x44, 0x44, 0x77 };
  ClutterColor   label_clr = { 0xff, 0xff, 0xff, 0xff };

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  switcher = clutter_group_new ();
  background = clutter_rectangle_new_with_color (&background_clr);

  label = clutter_label_new_full ("Sans 12", "Choose workspace:", &label_clr);
  clutter_actor_realize (label);

  grid = make_workspace_grid (G_CALLBACK (workspace_input_cb), NULL);
  clutter_actor_set_position (CLUTTER_ACTOR (grid), 0,
                              clutter_actor_get_height (label) + 3);

  clutter_container_add (CLUTTER_CONTAINER (switcher),
                         background, label, CLUTTER_ACTOR (grid), NULL);

  if (priv->workspace_switcher)
    hide_workspace_switcher ();

  priv->workspace_switcher = switcher;

  overlay = mutter_plugin_get_overlay_group (plugin);
  clutter_container_add_actor (CLUTTER_CONTAINER (overlay), switcher);

  clutter_actor_get_size (switcher, &switcher_width, &switcher_height);
  clutter_actor_set_size (background, switcher_width, switcher_height);

  clutter_actor_set_anchor_point (switcher,
                                  switcher_width/2, switcher_height/2);

  clutter_actor_set_position (switcher, screen_width/2, screen_height/2);

  mutter_plugin_set_stage_reactive (plugin, TRUE);
}

static void
toggle_workspace_switcher ()
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;

  if (priv->workspace_switcher)
    hide_workspace_switcher ();
  else
    show_workspace_switcher ();
}

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

static void
hide_workspace_chooser (void)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  PluginPrivate *priv   = plugin->plugin_private;

  if (!priv->workspace_chooser)
    return;

  clutter_actor_destroy (priv->workspace_chooser);

  disable_stage (plugin);

  priv->workspace_chooser = NULL;
}

static gboolean
workspace_chooser_input_cb (ClutterActor *clone,
                            ClutterEvent *event,
                            gpointer      data)
{
  MetaWorkspace *workspace = data;
  MutterPlugin  *plugin    = mutter_get_plugin ();
  PluginPrivate *priv      = plugin->plugin_private;

  if (!workspace)
    {
      g_warning ("No workspace specified, %s:%d\n", __FILE__, __LINE__);
      return FALSE;
    }

  priv->next_app_workspace = meta_workspace_index (workspace);

  hide_workspace_chooser ();

  spawn_app (priv->app_to_start);

  g_free (priv->app_to_start);
  priv->app_to_start = NULL;

  return FALSE;
}

static gboolean
new_workspace_input_cb (ClutterActor *clone,
                        ClutterEvent *event,
                        gpointer      data)
{
  MutterPlugin  *plugin    = mutter_get_plugin ();
  PluginPrivate *priv      = plugin->plugin_private;
  MetaScreen    *screen    = mutter_plugin_get_screen (plugin);

  priv->next_app_workspace = GPOINTER_TO_INT (data);

  hide_workspace_chooser ();

  meta_screen_append_new_workspace (screen, TRUE, event->any.time);
  spawn_app (priv->app_to_start);

  g_free (priv->app_to_start);
  priv->app_to_start = NULL;

  return FALSE;
}

static void
show_workspace_chooser (const gchar *app_path)
{
  MutterPlugin  *plugin   = mutter_get_plugin ();
  PluginPrivate *priv     = plugin->plugin_private;
  ClutterActor  *overlay;
  ClutterActor  *switcher;
  ClutterActor  *background;
  ClutterActor  *grid;
  ClutterActor  *new_ws;
  ClutterActor  *new_ws_background;
  ClutterActor  *new_ws_label;
  gint           screen_width, screen_height;
  gint           switcher_width, switcher_height;
  ClutterColor   background_clr = { 0x44, 0x44, 0x44, 0x77 };
  ClutterColor   new_ws_clr = { 0xff, 0xff, 0xff, 0xff };
  ClutterColor   new_ws_text_clr = { 0, 0, 0, 0xff };
  gint           ws_count = 0;

  g_free (priv->app_to_start);
  priv->app_to_start = g_strdup (app_path);

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  switcher = clutter_group_new ();
  background = clutter_rectangle_new_with_color (&background_clr);

  grid = make_workspace_grid (G_CALLBACK (workspace_chooser_input_cb),
                              &ws_count);

  new_ws = clutter_group_new ();
  new_ws_background = clutter_rectangle_new_with_color (&new_ws_clr);

  clutter_actor_set_size (new_ws_background,
                          WORKSPACE_CELL_WIDTH, WORKSPACE_CELL_HEIGHT);
  new_ws_label = clutter_label_new_full ("Sans 10", "New Workspace",
                                         &new_ws_text_clr);
  clutter_actor_set_anchor_point_from_gravity (new_ws_label,
                                               CLUTTER_GRAVITY_CENTER);
  clutter_actor_set_position (new_ws_label,
                              WORKSPACE_CELL_WIDTH / 2,
                              WORKSPACE_CELL_HEIGHT / 2);

  clutter_container_add (CLUTTER_CONTAINER (new_ws),
                         new_ws_background, new_ws_label, NULL);

  g_signal_connect (new_ws, "button-press-event",
                    G_CALLBACK (new_workspace_input_cb),
                    GINT_TO_POINTER (ws_count));

  clutter_actor_set_reactive (new_ws, TRUE);

  clutter_container_add_actor (CLUTTER_CONTAINER (grid), new_ws);

  clutter_actor_set_size (grid,
                          (ws_count+1) * (WORKSPACE_CELL_WIDTH+80),
                          WORKSPACE_CELL_HEIGHT);

  clutter_container_add (CLUTTER_CONTAINER (switcher),
                         background, CLUTTER_ACTOR (grid), NULL);

  if (priv->workspace_chooser)
    hide_workspace_chooser ();

  priv->workspace_chooser = switcher;

  overlay = mutter_plugin_get_overlay_group (plugin);

  clutter_container_add_actor (CLUTTER_CONTAINER (overlay), switcher);

  clutter_actor_get_size (switcher, &switcher_width, &switcher_height);
  clutter_actor_set_size (background, switcher_width, switcher_height);

  clutter_actor_set_anchor_point (switcher,
                                  switcher_width/2, switcher_height/2);

  clutter_actor_set_position (switcher, screen_width/2, screen_height/2);

  mutter_plugin_set_stage_reactive (plugin, TRUE);
}

static gboolean
switcher_clone_input_cb (ClutterActor *clone,
                         ClutterEvent *event,
                         gpointer      data)
{
  MutterWindow  *mw = data;
  MetaWindow    *window;
  MetaWorkspace *workspace;

  window    = mutter_window_get_meta_window (mw);
  workspace = meta_window_get_workspace (window);

  hide_switcher ();
  meta_workspace_activate_with_focus (workspace, window, event->any.time);

  return FALSE;
}

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

      if (priv->panel_out &&
          (event->type == CLUTTER_BUTTON_PRESS || !priv->switcher))
        {
          guint height = clutter_actor_get_height (priv->panel);
          gint  x      = clutter_actor_get_x (priv->panel);

          if (event_y > (gint)height)
            {
              priv->panel_back_in_progress  = TRUE;

              clutter_effect_move (priv->panel_slide_effect,
                                   priv->panel, x, -height,
                                   on_panel_effect_complete,
                                   GINT_TO_POINTER (FALSE));
              priv->panel_out = FALSE;
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
      else if (!priv->switcher)
        {
          gint screen_width, screen_height;

          mutter_plugin_query_screen_size (plugin,
                                           &screen_width, &screen_height);

          if (event_x > screen_width - WS_SWITCHER_SLIDE_THRESHOLD)
            toggle_workspace_switcher ();
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
  ClutterColor  bkg_clr = {0, 0, 0x7f, 0xff};
  ClutterColor  fg_clr  = {0xf8, 0xd9, 0x09, 0xff};
  ClutterActor *group = clutter_group_new ();
  ClutterActor *label = clutter_label_new_full ("Sans 12", name, &fg_clr);
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

  launcher = make_app_launcher ("Calculator", "/usr/bin/gcalctool");
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
  gint           screen_width, screen_height;
  XRectangle     rect[2];
  XserverRegion  region;
  Display       *xdpy = mutter_plugin_get_xdisplay (plugin);;

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

  panel = priv->panel = make_panel (screen_width);
  clutter_container_add_actor (CLUTTER_CONTAINER (overlay), panel);

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
