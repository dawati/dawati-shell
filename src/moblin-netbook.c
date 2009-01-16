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
#include "moblin-netbook-chooser.h"
#include "moblin-netbook-panel.h"
#include "mnb-drop-down.h"
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
#define PANEL_SLIDE_THRESHOLD       1
#define PANEL_SLIDE_THRESHOLD_TIMEOUT 300
#define WS_SWITCHER_SLIDE_TIMEOUT   250
#define ACTOR_DATA_KEY "MCCP-moblin-netbook-actor-data"

static gboolean stage_input_cb (ClutterActor *stage, ClutterEvent *event,
                                gpointer data);
static gboolean stage_capture_cb (ClutterActor *stage, ClutterEvent *event,
                                  gpointer data);

static void setup_parallax_effect (MutterPlugin *plugin);

static GQuark actor_data_quark = 0;

static void     minimize   (MutterPlugin *plugin,
                            MutterWindow *actor);
static void     map        (MutterPlugin *plugin,
                            MutterWindow *actor);
static void     destroy    (MutterPlugin *plugin,
                            MutterWindow *actor);
static void     maximize   (MutterPlugin *plugin,
                            MutterWindow *actor,
                            gint x, gint y, gint width, gint height);
static void     unmaximize (MutterPlugin *plugin,
                            MutterWindow *actor,
                            gint x, gint y, gint width, gint height);

static void     switch_workspace (MutterPlugin *plugin,
                                  const GList **actors, gint from, gint to,
                                  MetaMotionDirection direction);

static void     kill_effect (MutterPlugin *plugin,
                             MutterWindow *actor, gulong event);

static const MutterPluginInfo * plugin_info (MutterPlugin *plugin);

static gboolean xevent_filter (MutterPlugin *plugin, XEvent *xev);

MUTTER_PLUGIN_DECLARE (MoblinNetbookPlugin, moblin_netbook_plugin);

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
moblin_netbook_plugin_dispose (GObject *object)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (object)->priv;
  Display                    *xdpy;

  xdpy = mutter_plugin_get_xdisplay (MUTTER_PLUGIN (object));

  if (priv->screen_region)
    XFixesDestroyRegion (xdpy, priv->screen_region);

  if (priv->input_region)
    XFixesDestroyRegion (xdpy, priv->input_region);

#ifndef WORKING_STAGE_ENTER_LEAVE
  if (priv->input_region)
    XFixesDestroyRegion (xdpy, priv->input_region2);
#endif

  g_object_unref (priv->destroy_effect);
  g_object_unref (priv->minimize_effect);
  g_object_unref (priv->maximize_effect);
  g_object_unref (priv->switch_workspace_effect);
  g_object_unref (priv->switch_workspace_arrow_effect);

  G_OBJECT_CLASS (moblin_netbook_plugin_parent_class)->dispose (object);
}

static void
moblin_netbook_plugin_finalize (GObject *object)
{
  G_OBJECT_CLASS (moblin_netbook_plugin_parent_class)->finalize (object);
}

static void
moblin_netbook_plugin_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
moblin_netbook_plugin_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
moblin_netbook_plugin_constructed (GObject *object)
{
  MoblinNetbookPlugin        *plugin = MOBLIN_NETBOOK_PLUGIN (object);
  MoblinNetbookPluginPrivate *priv   = plugin->priv;

  guint destroy_timeout           = DESTROY_TIMEOUT;
  guint minimize_timeout          = MINIMIZE_TIMEOUT;
  guint maximize_timeout          = MAXIMIZE_TIMEOUT;
  guint map_timeout               = MAP_TIMEOUT;
  guint switch_timeout            = SWITCH_TIMEOUT;
  guint panel_slide_timeout       = PANEL_SLIDE_TIMEOUT;
  guint ws_switcher_slide_timeout = WS_SWITCHER_SLIDE_TIMEOUT;

  ClutterActor  *overlay;
  ClutterActor  *panel;
  ClutterActor  *lowlight;
  gint           screen_width, screen_height;
  XRectangle     rect[1];
  XserverRegion  region;
  Display       *xdpy = mutter_plugin_get_xdisplay (MUTTER_PLUGIN (plugin));
  ClutterColor   low_clr = { 0, 0, 0, 0x7f };
  GError        *err = NULL;
  MetaScreen    *screen;
  Window         root_xwin;

  gtk_init (NULL, NULL);

  screen    = mutter_plugin_get_screen (MUTTER_PLUGIN (plugin));
  root_xwin = RootWindow (xdpy, meta_screen_get_screen_number (screen));

  XGrabKey (xdpy, XKeysymToKeycode (xdpy, MOBLIN_PANEL_SHORTCUT_KEY),
            AnyModifier,
            root_xwin, True, GrabModeAsync, GrabModeAsync);

  /* tweak with env var as then possible to develop in desktop env. */
  if (!g_getenv("MUTTER_DISABLE_WS_CLAMP"))
    meta_prefs_set_num_workspaces (1);

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             PLUGIN_PKGDATADIR "/theme/mutter-moblin.css",
                             &err);
  if (err)
    {
      g_warning (err->message);
      g_error_free (err);
    }

  mutter_plugin_query_screen_size (MUTTER_PLUGIN (plugin),
                                   &screen_width, &screen_height);

  rect[0].x = 0;
  rect[0].y = 0;
  rect[0].width = screen_width;
  rect[0].height = PANEL_SLIDE_THRESHOLD;

  region = XFixesCreateRegion (xdpy, &rect[0], 1);

  priv->input_region = region;

#ifndef WORKING_STAGE_ENTER_LEAVE
  rect[0].height += 5;

  region = XFixesCreateRegion (xdpy, &rect[0], 1);

  priv->input_region2 = region;
#endif

  rect[0].height = screen_height;

  region = XFixesCreateRegion (xdpy, &rect[0], 1);

  priv->screen_region = region;

  if (mutter_plugin_debug_mode (MUTTER_PLUGIN (plugin)))
    {
      g_debug ("%s: Entering debug mode.", priv->info.name);

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

  overlay = mutter_plugin_get_overlay_group (MUTTER_PLUGIN (plugin));

  lowlight = clutter_rectangle_new_with_color (&low_clr);
  priv->lowlight = lowlight;
  clutter_actor_set_size (lowlight, screen_width, screen_height);

  /*
   * This also creates the launcher.
   */
  panel = priv->panel = make_panel (MUTTER_PLUGIN (plugin), screen_width);
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
  disable_stage (MUTTER_PLUGIN (plugin), CurrentTime);

  /*
   * Hook to the captured signal, so we get to see all events before our
   * children and do not interfere with their event processing.
   */
  g_signal_connect (mutter_plugin_get_stage (MUTTER_PLUGIN (plugin)),
                    "captured-event", G_CALLBACK (stage_capture_cb),
                    plugin);

  g_signal_connect (mutter_plugin_get_stage (MUTTER_PLUGIN (plugin)),
                    "button-press-event", G_CALLBACK (stage_input_cb),
                    plugin);
  g_signal_connect (mutter_plugin_get_stage (MUTTER_PLUGIN (plugin)),
                    "key-press-event", G_CALLBACK (stage_input_cb),
                    plugin);

  clutter_set_motion_events_enabled (TRUE);

  setup_parallax_effect (MUTTER_PLUGIN (plugin));

  setup_startup_notification (MUTTER_PLUGIN (plugin));

  moblin_netbook_notify_init (MUTTER_PLUGIN (plugin));
}

static void
moblin_netbook_plugin_class_init (MoblinNetbookPluginClass *klass)
{
  GObjectClass      *gobject_class = G_OBJECT_CLASS (klass);
  MutterPluginClass *plugin_class  = MUTTER_PLUGIN_CLASS (klass);

  gobject_class->finalize        = moblin_netbook_plugin_finalize;
  gobject_class->dispose         = moblin_netbook_plugin_dispose;
  gobject_class->constructed     = moblin_netbook_plugin_constructed;
  gobject_class->set_property    = moblin_netbook_plugin_set_property;
  gobject_class->get_property    = moblin_netbook_plugin_get_property;

  plugin_class->map              = map;
  plugin_class->minimize         = minimize;
  plugin_class->maximize         = maximize;
  plugin_class->unmaximize       = unmaximize;
  plugin_class->destroy          = destroy;
  plugin_class->switch_workspace = switch_workspace;
  plugin_class->kill_effect      = kill_effect;
  plugin_class->plugin_info      = plugin_info;
  plugin_class->xevent_filter    = xevent_filter;

  g_type_class_add_private (gobject_class, sizeof (MoblinNetbookPluginPrivate));
}

static void
moblin_netbook_plugin_init (MoblinNetbookPlugin *self)
{
  MoblinNetbookPluginPrivate *priv;

  self->priv = priv = MOBLIN_NETBOOK_PLUGIN_GET_PRIVATE (self);

  priv->info.name        = _("Moblin Netbook Effects");
  priv->info.version     = "0.1";
  priv->info.author      = "Intel Corp.";
  priv->info.license     = "GPL";
  priv->info.description = _("Effects for Moblin Netbooks");
}

static void
on_desktop_pre_paint (ClutterActor *actor, gpointer data)
{
  MoblinNetbookPlugin *plugin = data;
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
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

struct ws_switch_data
{
  GList **actors;
  MutterPlugin *plugin;
};

static void
on_switch_workspace_effect_complete (ClutterActor *group, gpointer data)
{
  struct ws_switch_data *switch_data = data;
  MutterPlugin   *plugin = switch_data->plugin;
  MoblinNetbookPluginPrivate *ppriv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  GList          *l      = *(switch_data->actors);
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

  startup_notification_finalize (plugin);

  g_free (switch_data);

  mutter_plugin_effect_completed (plugin, actor_for_cb,
                                  MUTTER_PLUGIN_SWITCH_WORKSPACE);
}

struct parallax_data
{
  gint direction;
  MoblinNetbookPlugin *plugin;
};

static void
on_workspace_frame_change (ClutterTimeline *timeline,
                           gint             frame_num,
                           gpointer         data)
{
  struct parallax_data *parallax_data = data;
  MoblinNetbookPluginPrivate  *priv  = parallax_data->plugin->priv;

  priv->parallax_paint_offset += parallax_data->direction * -1;
}

static void
switch_workspace (MutterPlugin *plugin, const GList **actors,
                  gint from, gint to,
                  MetaMotionDirection direction)
{
  MoblinNetbookPluginPrivate *ppriv  = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
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
  MetaScreen    *screen = mutter_plugin_get_screen (plugin);
  struct parallax_data *parallax_data = g_new (struct parallax_data, 1);
  struct ws_switch_data *switch_data = g_new (struct ws_switch_data, 1);

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

  n_workspaces = meta_screen_get_n_workspaces (screen);

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

  switch_data->actors = (GList**)actors;
  switch_data->plugin = plugin;

  /* workspace were going too */
  ppriv->tml_switch_workspace1 =
    clutter_effect_move (ppriv->switch_workspace_effect, workspace_slider1,
                         0, 0,
                         on_switch_workspace_effect_complete,
                         switch_data);
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
  parallax_data->direction = para_dir;
  parallax_data->plugin = MOBLIN_NETBOOK_PLUGIN (plugin);

  g_signal_connect_data (ppriv->tml_switch_workspace1,
                         "new-frame",
                         G_CALLBACK (on_workspace_frame_change),
                         parallax_data,
                         (GClosureNotify)g_free, 0);
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
  MutterPlugin *plugin = data;
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
minimize (MutterPlugin * plugin, MutterWindow *mcw)

{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
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
                                                  plugin);
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
  MutterPlugin *plugin = data;
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
maximize (MutterPlugin *plugin, MutterWindow *mcw,
          gint end_x, gint end_y, gint end_width, gint end_height)
{
  MoblinNetbookPluginPrivate *priv  = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  ClutterActor               *actor = CLUTTER_ACTOR (mcw);
  MetaCompWindowType          type;

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
                                                  plugin);

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
unmaximize (MutterPlugin *plugin, MutterWindow *mcw,
            gint end_x, gint end_y, gint end_width, gint end_height)
{
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

struct map_data
{
  MutterPlugin *plugin;
  ClutterActor *actor;
};

static void
on_map_effect_complete (ClutterTimeline *timeline, gpointer data)
{
  /*
   * Must reverse the effect of the effect.
   */
  struct map_data *map_data = data;
  MutterPlugin *plugin = map_data->plugin;
  MutterWindow *mcw    = MUTTER_WINDOW (map_data->actor);
  ActorPrivate *apriv  = get_actor_private (mcw);

  apriv->tml_map = NULL;

  clutter_actor_move_anchor_point_from_gravity (map_data->actor,
                                                CLUTTER_GRAVITY_NORTH_WEST);

  g_free (map_data);

  /* Now notify the manager that we are done with this effect */
  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
}

static void
on_config_actor_destroy (ClutterActor *actor, gpointer data)
{
  ClutterActor *background = data;

  clutter_actor_destroy (background);
}

struct config_map_data
{
  MutterPlugin *plugin;
  MutterWindow *mcw;
};

static void
on_config_actor_show_completed_cb (ClutterActor *actor, gpointer data)
{
  struct config_map_data *map_data = data;
  MutterPlugin           *plugin   = map_data->plugin;
  MutterWindow           *mcw      = map_data->mcw;

  g_free (map_data);

  /* Notify the manager that we are done with this effect */
  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
}

struct config_hide_data
{
  MoblinNetbookPlugin *plugin;
  Window               config_xwin;
};

static void
on_config_actor_hide_cb (ClutterActor *actor, gpointer data)
{
  struct config_hide_data *hide_data = data;

  shell_tray_manager_close_config_window (hide_data->plugin->priv->tray_manager,
                                          hide_data->config_xwin);
}

/*
 * Simple map handler: it applies a scale effect which must be reversed on
 * completion).
 */
static void
map (MutterPlugin *plugin, MutterWindow *mcw)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  ClutterActor               *actor = CLUTTER_ACTOR (mcw);
  MetaCompWindowType          type;

  type = mutter_window_get_window_type (mcw);

  if (type == META_COMP_WINDOW_DESKTOP && priv->parallax_tex != NULL )
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
                        plugin);

      mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
      return;
    }

  /*
   * The OR test must come first, since GTK_WINDOW_POPUP type windows are
   * both override redirect, but also have a _NET_WM_WINDOW_TYPE set to NORMAL
   */
  if (mutter_window_is_override_redirect (mcw))
    {
      Window xwin = mutter_window_get_x_window (mcw);

      if (shell_tray_manager_is_config_window (priv->tray_manager, xwin))
        {
          /*
           * Insert the actor into a custom frame, and then animate it to
           * position.
           *
           * Because of the way Mutter stacking we are unable to insert an actor
           * into the MutterWindow stack at an arbitrary position; instead
           * any actor we insert will end up on the very top. Additionally,
           * we do not want to reparent the MutterWindow, as that would
           * wreak havoc with the stacking.
           *
           * Consequently we have two options:
           *
           * (a) The center of our frame is transparent, and we overlay it over
           * the MutterWindow.
           *
           * (b) We reparent the actual glx texture inside Mutter window to
           * our frame, and destroy it manually when we close the window.
           *
           * We do (b).
           */
          struct config_map_data  *map_data;
          struct config_hide_data *hide_data;

          ClutterActor *background;
          ClutterActor *parent;
          ClutterActor *texture = mutter_window_get_texture (mcw);

          gint  x = clutter_actor_get_x (actor);
          gint  y = clutter_actor_get_y (actor);
          guint h = clutter_actor_get_height (texture);
          guint w = clutter_actor_get_width (texture);

          background = CLUTTER_ACTOR (mnb_drop_down_new ());

          g_object_ref (texture);
          clutter_actor_unparent (texture);
          mnb_drop_down_set_child (MNB_DROP_DOWN (background), texture);
          g_object_unref (texture);

          g_signal_connect (actor, "destroy",
                            G_CALLBACK (on_config_actor_destroy), background);

          map_data         = g_new (struct config_map_data, 1);
          map_data->plugin = plugin;
          map_data->mcw    = mcw;

          g_signal_connect (background, "show-completed",
                            G_CALLBACK (on_config_actor_show_completed_cb),
                            map_data);

          hide_data              = g_new (struct config_hide_data, 1);
          hide_data->plugin      = MOBLIN_NETBOOK_PLUGIN (plugin);
          hide_data->config_xwin = xwin;

          g_signal_connect_data (background, "hide",
                                 G_CALLBACK (on_config_actor_hide_cb),
                                 hide_data,
                                 (GClosureNotify)g_free, 0);

          clutter_actor_set_position (background, x, y);

          parent = clutter_actor_get_parent (actor);

          clutter_container_add_actor (CLUTTER_CONTAINER (parent), background);
          clutter_actor_show_all (background);
        }
      else
        mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);

    }
  else if (type == META_COMP_WINDOW_NORMAL ||
           type == META_COMP_WINDOW_SPLASHSCREEN)
    {
      ActorPrivate *apriv = get_actor_private (mcw);
      MetaWindow   *mw = mutter_window_get_meta_window (mcw);
      gint          workspace_index = -2;
      struct map_data *map_data;

      if (mw)
        {
          const char *sn_id = meta_window_get_startup_id (mw);

          if (!startup_notification_should_map (plugin, mcw, sn_id))
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

      map_data = g_new (struct map_data, 1);
      map_data->plugin = plugin;
      map_data->actor = actor;

      g_signal_connect (apriv->tml_map, "completed",
                        G_CALLBACK (on_map_effect_complete), map_data);

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
  MutterPlugin *plugin = data;
  MutterWindow *mcw    = MUTTER_WINDOW (actor);
  ActorPrivate *apriv  = get_actor_private (mcw);

  apriv->tml_destroy = NULL;

  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_DESTROY);
}

static void
check_for_empty_workspaces (MutterPlugin *plugin,
                            gint workspace, MutterWindow *ignore)
{
  MoblinNetbookPluginPrivate  *priv   = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                  *screen = mutter_plugin_get_screen (plugin);
  gboolean                     workspace_empty = TRUE;
  GList                        *l;

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
destroy (MutterPlugin *plugin, MutterWindow *mcw)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen;
  MetaCompWindowType          type;
  ClutterActor               *actor = CLUTTER_ACTOR (mcw);
  gint                        workspace;
  gboolean                    workspace_empty = TRUE;

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
                                                 plugin);
    }
  else
    mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_DESTROY);

  /*
   * Do not destroy workspace if the closing window is a splash screen.
   * (Sometimes the splash gets destroyed before the application window
   * maps, e.g., Gimp.)
   */
  if (type != META_COMP_WINDOW_SPLASHSCREEN)
    check_for_empty_workspaces (plugin, workspace, mcw);
}

/*
 * Use this function to disable stage input
 *
 * Used by the completion callback for the panel in/out effects
 */
void
disable_stage (MutterPlugin *plugin, guint32 timestamp)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  mutter_plugin_set_stage_input_region (MUTTER_PLUGIN (plugin),
                                        priv->input_region);

  if (priv->keyboard_grab)
    {
      if (timestamp == CurrentTime)
        {
          MetaScreen  *screen  =
            mutter_plugin_get_screen (MUTTER_PLUGIN (plugin));

          MetaDisplay *display = meta_screen_get_display (screen);

          timestamp = meta_display_get_current_time_roundtrip (display);
        }

      Display *xdpy = mutter_plugin_get_xdisplay (MUTTER_PLUGIN (plugin));

      XUngrabKeyboard (xdpy, timestamp);
      XSync (xdpy, False);
      priv->keyboard_grab = FALSE;
    }
}

void
enable_stage (MutterPlugin *plugin, guint32 timestamp)
{
  MoblinNetbookPluginPrivate *priv   = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen = mutter_plugin_get_screen (plugin);
  Display                    *xdpy   = mutter_plugin_get_xdisplay (plugin);
  ClutterActor               *stage  = mutter_get_stage_for_screen (screen);
  Window                      xwin;

  xwin   = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

  mutter_plugin_set_stage_reactive (MUTTER_PLUGIN (plugin), TRUE);

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
xevent_filter (MutterPlugin *plugin, XEvent *xev)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  if (xev->type == KeyPress &&
      XKeycodeToKeysym (xev->xkey.display, xev->xkey.keycode, 0) ==
                                                    MOBLIN_PANEL_SHORTCUT_KEY)
    {
      show_panel (plugin, TRUE);
      return TRUE;
    }

  sn_display_process_event (priv->sn_display, xev);

  clutter_x11_handle_event (xev);

  return FALSE;
}

static void
kill_effect (MutterPlugin *plugin, MutterWindow *mcw, gulong event)
{
  ActorPrivate *apriv;
  ClutterActor *actor = CLUTTER_ACTOR (mcw);

  if (event & MUTTER_PLUGIN_SWITCH_WORKSPACE)
    {
      MoblinNetbookPluginPrivate *ppriv  = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

      if (ppriv->tml_switch_workspace0)
        {
          clutter_timeline_stop (ppriv->tml_switch_workspace0);
          clutter_timeline_stop (ppriv->tml_switch_workspace1);

          /*
           * Force emission of the "completed" signal so that the necessary
           * cleanup is done (we cannot readily supply the data necessary to
           * call our callback directly).
           */
          g_signal_emit_by_name (ppriv->tml_switch_workspace1, "completed");
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
      on_minimize_effect_complete (actor, plugin);
    }

  if ((event & MUTTER_PLUGIN_MAXIMIZE) && apriv->tml_maximize)
    {
      clutter_timeline_stop (apriv->tml_maximize);
      on_maximize_effect_complete (actor, plugin);
    }

  if ((event & MUTTER_PLUGIN_MAP) && apriv->tml_map)
    {
      clutter_timeline_stop (apriv->tml_map);

      /*
       * Force emission of the "completed" signal so that the necessary
       * cleanup is done (we cannot readily supply the data necessary to
       * call our callback directly).
       */
      g_signal_emit_by_name (apriv->tml_map, "completed");
    }

  if ((event & MUTTER_PLUGIN_DESTROY) && apriv->tml_destroy)
    {
      clutter_timeline_stop (apriv->tml_destroy);
      on_destroy_effect_complete (actor, plugin);
    }
}

static gboolean
panel_slide_timeout_cb (gpointer data)
{
  MutterPlugin  *plugin = data;
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  int i;

#ifndef WORKING_STAGE_ENTER_LEAVE
  printf ("last_y %d\n", priv->last_y);

  if (priv->last_y < PANEL_SLIDE_THRESHOLD)
    {
      show_panel (plugin, FALSE);
    }
  else
    {
      disable_stage (plugin, CurrentTime);
    }
#else
  if (priv->pointer_on_stage)
    show_panel (plugin, FALSE);
#endif

  priv->panel_slide_timeout_id = 0;

  return FALSE;
}

static gboolean
stage_capture_cb (ClutterActor *stage, ClutterEvent *event, gpointer data)
{
  MoblinNetbookPlugin *plugin = data;

  if (event->type == CLUTTER_MOTION)
    {
      gint                        event_y, event_x;
      MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

      event_x = ((ClutterMotionEvent*)event)->x;
      event_y = ((ClutterMotionEvent*)event)->y;

#ifndef WORKING_STAGE_ENTER_LEAVE
      priv->last_y = event_y;
#endif

      if (priv->panel_out_in_progress || priv->panel_back_in_progress)
        return FALSE;

      if (priv->panel_out &&
          ((!priv->switcher && !priv->workspace_switcher &&
            !CLUTTER_ACTOR_IS_VISIBLE (priv->launcher) &&
            !CLUTTER_ACTOR_IS_VISIBLE (priv->mzone_grid))))
        {
          guint height = clutter_actor_get_height (priv->panel_shadow);

          if (event_y > (gint)height)
            {
              hide_panel (MUTTER_PLUGIN (plugin));
            }
        }
      else if (event_y < PANEL_SLIDE_THRESHOLD)
        {
          if (!priv->panel_slide_timeout_id)
            {
#ifndef WORKING_STAGE_ENTER_LEAVE
              mutter_plugin_set_stage_input_region (MUTTER_PLUGIN (plugin),
                                                    priv->input_region2);
#endif
              priv->panel_slide_timeout_id =
                g_timeout_add (PANEL_SLIDE_THRESHOLD_TIMEOUT,
                               panel_slide_timeout_cb, plugin);
            }
        }
    }
  else if (event->any.source == stage &&
           event->type == CLUTTER_ENTER || event->type == CLUTTER_LEAVE)
    {
      MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

      if (event->type == CLUTTER_ENTER)
        priv->pointer_on_stage = TRUE;
      else
        priv->pointer_on_stage = FALSE;

      return FALSE;
    }

  return FALSE;
}

/*
 * Handles input events on stage.
 *
 */
static gboolean
stage_input_cb (ClutterActor *stage, ClutterEvent *event, gpointer data)
{
  MutterPlugin *plugin = data;

  if (event->type == CLUTTER_BUTTON_PRESS)
    {
      gint           event_y, event_x;
      MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

      event_x = ((ClutterButtonEvent*)event)->x;
      event_y = ((ClutterButtonEvent*)event)->y;

      priv->last_y = event_y;

      if (priv->panel_out_in_progress || priv->panel_back_in_progress)
        return FALSE;

      if (priv->workspace_switcher)
        hide_workspace_switcher ();

      if (priv->launcher)
        clutter_actor_hide (priv->launcher);

      if (priv->mzone_grid)
        clutter_actor_hide (priv->mzone_grid);

      if (priv->panel_out)
        {
          guint height = clutter_actor_get_height (priv->panel_shadow);

          if (event_y > (gint)height)
            {
              hide_panel (plugin);
            }
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
setup_parallax_effect (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);
  Display                    *xdpy = mutter_plugin_get_xdisplay (plugin);
  gboolean                    have_desktop = FALSE;
  gint                        screen_width, screen_height;
  Window                     *children, *l;
  guint                       n_children;
  Window                      root_win;
  Window                      parent_win, root_win2;
  Status                      status;
  Atom                        desktop_atom;

  mutter_plugin_query_screen_size (MUTTER_PLUGIN (plugin),
                                   &screen_width, &screen_height);

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
static const MutterPluginInfo *
plugin_info (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  return &priv->info;
}
