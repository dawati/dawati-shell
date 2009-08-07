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

#include "moblin-netbook.h"
#include "mnb-drop-down.h"
#include "mnb-switcher.h"
#include "mnb-toolbar.h"
#include "effects/mnb-switch-zones-effect.h"

#include <glib/gi18n.h>
#include <gdk/gdkx.h>

#include <moblin-panel/mpl-panel-common.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gmodule.h>
#include <string.h>
#include <signal.h>

#include <compositor-mutter.h>
#include <display.h>
#include <prefs.h>
#include <keybindings.h>

#define MINIMIZE_TIMEOUT            250
#define MAXIMIZE_TIMEOUT            250
#define MAP_TIMEOUT                 350
#define SWITCH_TIMEOUT              400
#define WS_SWITCHER_SLIDE_TIMEOUT   250
#define ACTOR_DATA_KEY "MCCP-moblin-netbook-actor-data"

static MutterPlugin *plugin_singleton = NULL;

/* callback data for when animations complete */
typedef struct
{
  ClutterActor *actor;
  MutterPlugin *plugin;
} EffectCompleteData;

static void setup_desktop_background (MutterPlugin *plugin);

static void setup_focus_window (MutterPlugin *plugin);

static void fullscreen_app_added (MoblinNetbookPluginPrivate *, gint);
static void fullscreen_app_removed (MoblinNetbookPluginPrivate *, gint);

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

static void     kill_effect (MutterPlugin *plugin,
                             MutterWindow *actor, gulong event);

static const MutterPluginInfo * plugin_info (MutterPlugin *plugin);

static gboolean xevent_filter (MutterPlugin *plugin, XEvent *xev);

MUTTER_PLUGIN_DECLARE (MoblinNetbookPlugin, moblin_netbook_plugin);

static void moblin_netbook_input_region_apply (MutterPlugin *plugin);

static gboolean
on_lowlight_button_event (ClutterActor *actor,
                          ClutterEvent *event,
                          gpointer      user_data);


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


  if (priv->current_input_region)
    {
      XFixesDestroyRegion (xdpy, priv->current_input_region);
      priv->current_input_region = None;
    }

  if (priv->toolbar)
    {
      clutter_actor_destroy (priv->toolbar);
      priv->toolbar = NULL;
    }

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
sync_notification_input_region_cb (ClutterActor        *notify_actor,
                                   MoblinNetbookPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv   = plugin->priv;
  MnbInputRegion             *region;

  if (notify_actor == priv->notification_urgent)
    region = &priv->notification_urgent_input_region;
  else
    region = &priv->notification_cluster_input_region;

  if (*region != NULL)
    {
      moblin_netbook_input_region_remove (MUTTER_PLUGIN(plugin), *region);
      *region = NULL;
    }

  if (CLUTTER_ACTOR_IS_MAPPED (notify_actor))
    {
      gfloat x,y;
      gfloat width,height;

      clutter_actor_get_transformed_position (notify_actor, &x, &y);
      clutter_actor_get_transformed_size (notify_actor, &width, &height);

      if (width != 0 && height != 0)
        {
          *region = moblin_netbook_input_region_push (MUTTER_PLUGIN(plugin),
                                                      x, y, width, height);
        }
    }
}

static void
on_urgent_notifiy_visible_cb (ClutterActor    *notify_urgent,
                              GParamSpec      *pspec,
                              MutterPlugin *plugin)
{
  moblin_netbook_set_lowlight (plugin,
                               CLUTTER_ACTOR_IS_MAPPED(notify_urgent));
}

static void
moblin_netbook_plugin_constructed (GObject *object)
{
  MoblinNetbookPlugin        *plugin = MOBLIN_NETBOOK_PLUGIN (object);
  MoblinNetbookPluginPrivate *priv   = plugin->priv;

  guint minimize_timeout          = MINIMIZE_TIMEOUT;
  guint maximize_timeout          = MAXIMIZE_TIMEOUT;
  guint map_timeout               = MAP_TIMEOUT;
  guint switch_timeout            = SWITCH_TIMEOUT;

  ClutterActor  *overlay;
  ClutterActor  *toolbar;
  ClutterActor  *lowlight;
  gint           screen_width, screen_height;
  ClutterColor   low_clr = { 0, 0, 0, 0x7f };
  GError        *err = NULL;
  MoblinNetbookNotifyStore *notify_store;

  plugin_singleton = (MutterPlugin*)object;

  /* tweak with env var as then possible to develop in desktop env. */
  if (!g_getenv("MUTTER_DISABLE_WS_CLAMP"))
    meta_prefs_set_num_workspaces (1);

  nbtk_texture_cache_load_cache (nbtk_texture_cache_get_default (),
                                 THEMEDIR "/nbtk.cache");
  nbtk_style_load_from_file (nbtk_style_get_default (),
                             THEMEDIR "/mutter-moblin.css",
                             &err);
  if (err)
    {
      g_warning ("%s", err->message);
      g_error_free (err);
    }

  mutter_plugin_query_screen_size (MUTTER_PLUGIN (plugin),
                                   &screen_width, &screen_height);

  if (mutter_plugin_debug_mode (MUTTER_PLUGIN (plugin)))
    {
      g_debug ("%s: Entering debug mode.", priv->info.name);
      /*
       * Double the effect duration to make them easier to observe.
       */
      minimize_timeout          *= 2;
      maximize_timeout          *= 2;
      map_timeout               *= 2;
      switch_timeout            *= 2;
    }

  overlay = mutter_plugin_get_overlay_group (MUTTER_PLUGIN (plugin));

  lowlight = clutter_rectangle_new_with_color (&low_clr);
  priv->lowlight = lowlight;
  clutter_actor_set_size (lowlight, screen_width, screen_height);
  clutter_actor_set_reactive (lowlight, TRUE);

  g_signal_connect (priv->lowlight, "captured-event",
                    G_CALLBACK (on_lowlight_button_event),
                    NULL);
  /*
   * This also creates the launcher.
   */
  toolbar = priv->toolbar =
    CLUTTER_ACTOR (mnb_toolbar_new (MUTTER_PLUGIN (plugin)));

#if 1
  mnb_toolbar_append_panel_old (MNB_TOOLBAR (toolbar),
                                MPL_PANEL_ZONES, _("zones"));
#endif

  clutter_set_motion_events_enabled (TRUE);

  setup_desktop_background (MUTTER_PLUGIN (plugin));

  setup_focus_window (MUTTER_PLUGIN (plugin));

  /* Notifications */
  notify_store = moblin_netbook_notify_store_new ();

  priv->notification_cluster = mnb_notification_cluster_new ();

  mnb_notification_cluster_set_store
                    (MNB_NOTIFICATION_CLUSTER(priv->notification_cluster),
                     notify_store);

  clutter_actor_set_anchor_point_from_gravity (priv->notification_cluster,
                                               CLUTTER_GRAVITY_SOUTH_EAST);

  clutter_actor_set_position (priv->notification_cluster,
                              screen_width,
                              screen_height);

  g_signal_connect (priv->notification_cluster,
                    "sync-input-region",
                    G_CALLBACK (sync_notification_input_region_cb),
                    MUTTER_PLUGIN (plugin));


  priv->notification_urgent = mnb_notification_urgent_new ();

  clutter_actor_set_anchor_point_from_gravity (priv->notification_urgent,
                                               CLUTTER_GRAVITY_CENTER);

  clutter_actor_set_position (priv->notification_urgent,
                              screen_width/2,
                              screen_height/2);

  mnb_notification_urgent_set_store
                        (MNB_NOTIFICATION_URGENT(priv->notification_urgent),
                         notify_store);

  g_signal_connect (priv->notification_urgent,
                    "sync-input-region",
                    G_CALLBACK (sync_notification_input_region_cb),
                    MUTTER_PLUGIN (plugin));

  g_signal_connect (priv->notification_urgent,
                    "notify::visible",
                    G_CALLBACK (on_urgent_notifiy_visible_cb),
                    MUTTER_PLUGIN (plugin));

  /*
   * Order matters:
   *
   *  - toolbar hint is below the toolbar (i.e., not visible if panel showing.
   *  - lowlight is above everything except urgent notifications
   */
  clutter_container_add (CLUTTER_CONTAINER (overlay),
                         toolbar,
                         priv->notification_cluster,
                         lowlight,
                         priv->notification_urgent,
                         NULL);

  clutter_actor_hide (lowlight);
  clutter_actor_hide (CLUTTER_ACTOR(priv->notification_urgent));

  /* Keys */

  meta_prefs_override_no_tab_popup (TRUE);
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
  plugin_class->switch_workspace = mnb_switch_zones_effect;
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

  bindtextdomain (GETTEXT_PACKAGE, PLUGIN_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
}

/*
 * Minimize effect completion callback; this function restores actor state, and
 * calls the manager callback function.
 */
static void
on_minimize_effect_complete (ClutterTimeline *timeline, EffectCompleteData *data)
{
  /*
   * Must reverse the effect of the effect; must hide it first to ensure
   * that the restoration will not be visible.
   */
  MutterPlugin *plugin = data->plugin;
  ActorPrivate *apriv;
  MutterWindow *mcw = MUTTER_WINDOW (data->actor);

  apriv = get_actor_private (mcw);
  apriv->tml_minimize = NULL;

  clutter_actor_hide (data->actor);

  clutter_actor_set_scale (data->actor, 1.0, 1.0);
  clutter_actor_move_anchor_point_from_gravity (data->actor,
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
  MetaCompWindowType type;
  ClutterActor      *actor  = CLUTTER_ACTOR (mcw);

  type = mutter_window_get_window_type (mcw);

  if (type == META_COMP_WINDOW_NORMAL)
    {
      ActorPrivate *apriv = get_actor_private (mcw);
      ClutterAnimation *animation;
      EffectCompleteData *data = g_new0 (EffectCompleteData, 1);

      apriv->is_minimized = TRUE;

      clutter_actor_move_anchor_point_from_gravity (actor,
                                                    CLUTTER_GRAVITY_CENTER);

      animation = clutter_actor_animate (actor,
                                         CLUTTER_EASE_IN_SINE,
                                         MINIMIZE_TIMEOUT,
                                         "scale-x", 0.0,
                                         "scale-y", 0.0,
                                         NULL);

      data->actor = actor;
      data->plugin = plugin;

      g_signal_connect (clutter_animation_get_timeline (animation),
                        "completed",
                        G_CALLBACK (on_minimize_effect_complete),
                        data);
    }
  else
    mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MINIMIZE);
}

/*
 * Minimize effect completion callback; this function restores actor state, and
 * calls the manager callback function.
 */
static void
on_maximize_effect_complete (ClutterTimeline *timeline, EffectCompleteData *data)
{
  /*
   * Must reverse the effect of the effect.
   */
  MutterPlugin *plugin = data->plugin;
  MutterWindow *mcw    = MUTTER_WINDOW (data->actor);
  ActorPrivate *apriv  = get_actor_private (mcw);

  apriv->tml_maximize = NULL;

  clutter_actor_set_scale (data->actor, 1.0, 1.0);
  clutter_actor_move_anchor_point_from_gravity (data->actor,
                                                CLUTTER_GRAVITY_NORTH_WEST);

  /* Now notify the manager that we are done with this effect */
  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAXIMIZE);

  g_free (data);
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
  ClutterActor               *actor = CLUTTER_ACTOR (mcw);
  MetaCompWindowType          type;

  gdouble  scale_x  = 1.0;
  gdouble  scale_y  = 1.0;
  gfloat   anchor_x = 0;
  gfloat   anchor_y = 0;

  type = mutter_window_get_window_type (mcw);

  if (type == META_COMP_WINDOW_NORMAL)
    {
      ActorPrivate *apriv = get_actor_private (mcw);
      ClutterAnimation *animation;
      EffectCompleteData *data = g_new0 (EffectCompleteData, 1);
      gfloat width, height;
      gfloat x, y;

      apriv->is_maximized = TRUE;

      clutter_actor_get_size (actor, &width, &height);
      clutter_actor_get_position (actor, &x, &y);

      /*
       * Work out the scale and anchor point so that the window is expanding
       * smoothly into the target size.
       */
      scale_x = (gdouble)end_width / (gdouble) width;
      scale_y = (gdouble)end_height / (gdouble) height;

      anchor_x = (gfloat)(x - end_x)* width /
        ((gfloat)(end_width - width));
      anchor_y = (gfloat)(y - end_y)* height /
        ((gfloat)(end_height - height));

      clutter_actor_move_anchor_point (actor, anchor_x, anchor_y);

      animation = clutter_actor_animate (actor,
                                         CLUTTER_EASE_IN_SINE,
                                         MAXIMIZE_TIMEOUT,
                                         "scale-x", scale_x,
                                         "scale-y", scale_y,
                                         NULL);

      data->actor = actor;
      data->plugin = plugin;

      g_signal_connect (clutter_animation_get_timeline (animation),
                        "completed",
                        G_CALLBACK (on_maximize_effect_complete),
                        data);
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

static void
moblin_netbook_move_window_to_workspace (MutterWindow *mcw,
                                         gint          workspace_index,
                                         guint32       timestamp)
{
  MetaWindow  *mw      = mutter_window_get_meta_window (mcw);
  MetaScreen  *screen  = meta_window_get_screen (mw);

  g_return_if_fail (mw && workspace_index > -2);

  if (mw)
    {
      MetaWorkspace * active_workspace;
      MetaWorkspace * workspace = meta_window_get_workspace (mw);
      gint            active_index = -2;

      active_workspace = meta_screen_get_active_workspace (screen);

      if (active_workspace)
        active_index = meta_workspace_index (active_workspace);

      /*
       * Move the window to the requested workspace; if the window is not
       * sticky, activate the workspace as well.
       */
      meta_window_change_workspace_by_index (mw, workspace_index, TRUE,
                                             timestamp);

      if (workspace_index == active_index)
        {
          meta_window_activate_with_workspace (mw, timestamp, workspace);
        }
      else if (workspace_index > -1)
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

static void
moblin_netbook_move_window_to_its_workspace (MutterPlugin *plugin,
                                             MutterWindow *mcw,
                                             guint32       timestamp)
{
  MetaScreen *screen = mutter_plugin_get_screen (plugin);
  gint index;
  gint n_workspaces;
  gboolean append = TRUE;

  n_workspaces = meta_screen_get_n_workspaces (screen);

  if (n_workspaces == 1)
    {
      gboolean  workspace_empty = TRUE;
      GList    *l;

      /*
       * Mutter now treats all OR windows as sticky, and the -1 will trigger
       * false workspace switch.
       */
      l = mutter_get_windows (screen);

      while (l)
        {
          MutterWindow       *m    = l->data;
          MetaWindow         *mw   = mutter_window_get_meta_window (m);
          MetaCompWindowType  type = mutter_window_get_window_type (m);

          if ((type == META_COMP_WINDOW_NORMAL) &&
              !mutter_window_is_override_redirect (m) &&
              !meta_window_is_on_all_workspaces (mw))
            {
              workspace_empty = FALSE;
              break;
            }

          l = l->next;
        }

      if (workspace_empty)
        {
          index = 0;
          append = FALSE;
        }
      else
        index = n_workspaces;
    }
  else if (n_workspaces >= MAX_WORKSPACES)
    {
      index = MAX_WORKSPACES - 1;
      append = FALSE;
    }
  else
    {
      index = n_workspaces;
    }

  if (append)
    {
      if (!meta_screen_append_new_workspace (screen, FALSE, timestamp))
        {
          g_warning ("Unable to append new workspace\n");
          return;
        }
    }

  moblin_netbook_move_window_to_workspace (mcw, index, timestamp);
}

static void
on_map_effect_complete (ClutterTimeline *timeline, EffectCompleteData *data)
{
  /*
   * Must reverse the effect of the effect.
   */
  MutterPlugin *plugin = data->plugin;
  MutterWindow *mcw    = MUTTER_WINDOW (data->actor);
  ActorPrivate *apriv  = get_actor_private (mcw);

  apriv->tml_map = NULL;

  clutter_actor_move_anchor_point_from_gravity (data->actor,
                                                CLUTTER_GRAVITY_NORTH_WEST);

  g_free (data);

  /* Now notify the manager that we are done with this effect */
  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
}

static void
check_for_empty_workspace (MutterPlugin *plugin,
                           gint workspace, MetaWindow *ignore)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen = mutter_plugin_get_screen (plugin);
  gboolean                    workspace_empty = TRUE;
  gboolean                    no_apps = TRUE;
  GList                      *l;

  /*
   * Mutter now treats all OR windows as sticky, and the -1 will trigger
   * false workspace switch.
   */
  if (workspace < 0)
    return;

  l = mutter_get_windows (screen);

  while (l)
    {
      MutterWindow *m = l->data;
      MetaWindow   *mw = mutter_window_get_meta_window (m);

      /*
       * We need to check this window is not the window we are too ignore, and
       * also not transient (e.g., dialog) of the window we are to ignore
       * (because when a window is moved to a different workspace, the WM
       * automatically moves its transients too).
       *
       * We skip over windows that are on all workspaces (they are irrelevant
       * for our purposes, and also because their workspace is the active
       * workspace, we can have a bit of a race here if workspace is being
       * removed when the screen active workspace is not yet updated at this
       * point and we we try to translate it to an index, it blows up on us).
       */
      if (mw != ignore &&
          !meta_window_is_on_all_workspaces (mw) &&
          !meta_window_is_ancestor_of_transient (ignore, mw))
        {
          g_debug ("querying workspace for [%s]",
                   mutter_window_get_description (m));

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
      MetaWorkspace  *current_ws;
      guint32         timestamp;
      gint            next_index = -1;

      timestamp  = clutter_x11_get_current_event_time ();
      current_ws = meta_screen_get_workspace_by_index (screen, workspace);

      /*
       * We need to activate the next workspace before we remove this one, so
       * that the zone switch effect works.
       */
      if (workspace > 0)
        next_index = workspace - 1;
      else if (meta_screen_get_n_workspaces (screen) > 1)
        next_index = workspace + 1;

      if (next_index != -1)
        {
          MetaWorkspace  *next_ws;
          next_ws = meta_screen_get_workspace_by_index (screen, next_index);

          if (!next_ws)
            {
              g_warning ("%s:%d: No workspace for index %d\n",
                         __FILE__, __LINE__, next_index);
            }
          else
            {
              meta_workspace_activate (next_ws, timestamp);
            }
        }

      meta_screen_remove_workspace (screen, current_ws, timestamp);
    }

  /*
   * Now check for running applications; we do this by checking if any
   * application-type windows are present.
   */
  l = mutter_get_windows (screen);

  while (l)
    {
      MutterWindow       *m    = l->data;
      MetaCompWindowType  type = mutter_window_get_window_type (m);
      MetaWindow         *mw;

      /*
       * Ignore desktop, docs, and panel windows
       *
       * (Panel windows are currently of type META_COMP_WINDOW_OVERRIDE_OTHER)
       */
      if (!(type == META_COMP_WINDOW_DESKTOP        ||
            type == META_COMP_WINDOW_DOCK           ||
            type == META_COMP_WINDOW_OVERRIDE_OTHER ||
            ((mw = mutter_window_get_meta_window (m)) == ignore)))
        {
          g_debug ("Found singificant window %s of type %d",
                   mutter_window_get_description (m), type);

          no_apps = FALSE;
          break;
        }

      l = l->next;
    }

  if (no_apps)
    mnb_toolbar_activate_panel (MNB_TOOLBAR (priv->toolbar), MPL_PANEL_MYZONE);
}

/*
 * Protype; don't want to add this the public includes in metacity,
 * should be able to get rid of this call eventually.
 */
void meta_window_calc_showing (MetaWindow  *window);

static void
meta_window_workspace_changed_cb (MetaWindow *mw,
                                  gint        old_workspace,
                                  gpointer    data)
{
  MutterPlugin               *plugin = MUTTER_PLUGIN (data);
  MoblinNetbookPluginPrivate *priv   = MOBLIN_NETBOOK_PLUGIN (data)->priv;
  gboolean                    fullscreen;

  g_object_get (mw, "fullscreen", &fullscreen, NULL);

  if (fullscreen)
    {
      MetaWorkspace *ws;

      ws = meta_window_get_workspace (mw);

      if (ws)
        {
          gint index = meta_workspace_index (ws);

          fullscreen_app_added (priv, index);
        }

      fullscreen_app_removed (priv, old_workspace);
    }

  /*
   * Flush any pending changes to the visibility of the window.
   * (bug 1008 suggests that the removal of an empty workspace is sometimes
   * causing a race condition on calculating the window visibility, along the
   * lines of the window changing status
   *
   *  visible -> hidden -> visible
   *
   * As this is queued up, by the time the status is calculated this might
   * appear as the visibility has not changed, but in fact somewhere along the
   * line it the window has already been pushed down the stack.
   *
   * Needs further investigation; this is an attempt to work around the problem
   * by flushing the state in the intermediate stage for the alpha2 release.
   */
  meta_window_calc_showing (mw);

  check_for_empty_workspace (plugin, old_workspace, mw);
}

static void
fullscreen_app_added (MoblinNetbookPluginPrivate *priv, gint workspace)
{
  if (workspace >= MAX_WORKSPACES)
    {
      g_warning ("There should be no workspace %d", workspace);
      return;
    }

  /* for sticky apps translate -1 to the sticky counter index */
  if (workspace < 0)
    workspace = MAX_WORKSPACES;

  priv->fullscreen_apps[workspace]++;
}

static void
fullscreen_app_removed (MoblinNetbookPluginPrivate *priv, gint workspace)
{
  if (workspace >= MAX_WORKSPACES)
    {
      g_warning ("There should be no workspace %d", workspace);
      return;
    }

  /* for sticky apps translate -1 to the sticky counter index */
  if (workspace < 0)
    workspace = MAX_WORKSPACES;

  priv->fullscreen_apps[workspace]--;

  if (priv->fullscreen_apps[workspace] < 0)
    {
      g_warning ("%s:%d: Error in fullscreen app accounting !!!",
                 __FILE__, __LINE__);
      priv->fullscreen_apps[workspace] = 0;
    }
}

gboolean
moblin_netbook_fullscreen_apps_present (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv   = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen = mutter_plugin_get_screen (plugin);
  gint                        active;

  active = meta_screen_get_active_workspace_index (screen);

  if (active >= MAX_WORKSPACES)
    {
      g_warning ("There should be no workspace %d", active);
      return FALSE;
    }

  if (active < 0)
    active = MAX_WORKSPACES;

  return (gboolean) priv->fullscreen_apps[active];
}

static void
meta_window_fullcreen_notify_cb (GObject    *object,
                                 GParamSpec *spec,
                                 gpointer    data)
{
  MetaWindow                 *mw   = META_WINDOW (object);
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (data)->priv;
  MetaWorkspace              *ws;
  gint                        ws_index;
  gboolean                    fullscreen;

  ws = meta_window_get_workspace (mw);

  if (!ws)
    return;

  ws_index = meta_workspace_index (ws);

  g_object_get (object, "fullscreen", &fullscreen, NULL);

  if (fullscreen)
    fullscreen_app_added (priv, ws_index);
  else
    fullscreen_app_removed (priv, ws_index);
}


/*
 * Temporary cludge to facilitate some rudimentary IM functionality
 *
 * The basic problem is that the IM is implemented using X windows for the
 * preview, toolbar, and various other UI elements; this is fine for regular
 * applications, but does not work for UX Shell UI (the UI is always above
 * the windows, so, the IM windows are covered and non-interactive.
 *
 * To work around this, when a window of WM_CLASS Scim-bridge-gtk maps, we
 * create a clone and places to the top of the stage. The preview window is
 * interactive by the virtue of overlapping the panels in it's entirety (and
 * naturally sitting above the panel windows in the X stacking order). For the
 * toolbar we force the visible panel to shring to free the bottom 45 pixels,
 * and reduce the Panel input region to end at the bottom of the footer. IM
 * toolbar menus work only to the extent they overlap with the inner panel area,
 * the bits that overalap with the panel footer are not reactive, because the
 * footer occludes the window.
 *
 * NB: This is not a solution; it's a cludge hacked together at the last moment.
 *     Proper solution, starting with interaction design is needed -- it is
 *     most likely that any future implementaion will need to integrate the IM
 *     UI elements directly into the compositor stage using Clutter, rather than
 *     using X windows.
 */

/*
 * When the real window moves, move the clone.
 */
static void
scim_preview_allocation_cb (ClutterActor *source,
                            GParamSpec   *pspec,
                            ClutterActor *clone)
{
  gfloat x, y, w, h;

  clutter_actor_get_position (source, &x, &y);
  clutter_actor_get_size (source, &w, &h);

  clutter_actor_set_position (clone, x, y);
  clutter_actor_set_size (clone, w, h);
}
/*
 * When the original window is destroyed, destroy the clone.
 */
static void
scim_preview_destroy_cb (ClutterActor *source, ClutterActor *clone)
{
  ClutterActor *parent = clutter_actor_get_parent (clone);

  if (parent)
    clutter_container_remove_actor (CLUTTER_CONTAINER (parent), clone);
  else
    clutter_actor_destroy (clone);
}

static void
scim_preview_raised_cb (MetaWindow *mw, ClutterActor *clone)
{
  clutter_actor_raise_top (clone);
}

/*
 * The following functions are trying to catch out possible corner cases where
 * our clone is left unpainted. Unfortunately, this still happens.
 */

/*
 * When the inner texture alloation changes, make sure to queue redraw of the
 * clone. (Should be unnecessary.)
 */
static void
scim_preview_texture_allocation_cb (ClutterActor *source,
                                    GParamSpec   *pspec,
                                    ClutterActor *clone)
{
  clutter_actor_queue_redraw (clone);
}

/*
 * Make sure that any queued redraws on the inner texture get propagated to
 * our clone. (Should be unnecessary.)
 */
static void
scim_preview_queue_redraw_cb (ClutterActor *source,
                              ClutterActor *origin,
                              ClutterActor *clone)
{
  clutter_actor_queue_redraw (clone);
}

/*
 * Helper function to resize active panel in presence of SCIM windows.
 *
 * The panel parameter can be NULL, in which case the active panel will be
 * looked up first.
 */
static void
scim_resize_active_panel (MutterPlugin *plugin, MnbPanel *panel)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MnbToolbar                 *toolbar = MNB_TOOLBAR (priv->toolbar);
  gfloat                      w, h;
  gint                        screen_width, screen_height;

  if (!panel &&
      !(panel = (MnbPanel*)mnb_toolbar_get_active_panel (toolbar)))
    return;

  mutter_plugin_query_screen_size (MUTTER_PLUGIN (plugin),
                                   &screen_width, &screen_height);

  clutter_actor_get_size (CLUTTER_ACTOR (panel), &w, &h);

  /*
   * Indicate to the toolbar that the region below panel footer
   * should not be included in the panel input region.
   */
  mnb_toolbar_set_panel_input_only (toolbar, TRUE);

  /*
   * If the panel stretches all the way to the bottom of the screen
   * shrink it, so that the IM toolbar window becomes visible.
   */
  if ((guint)h >= screen_height + 34 - TOOLBAR_HEIGHT)
    {
      if (MNB_IS_PANEL (panel))
        mnb_panel_set_size (panel,
                            (guint) w, screen_height + 37 - 45 -
                            TOOLBAR_HEIGHT);
      else
        clutter_actor_set_size (CLUTTER_ACTOR (panel),
                                w,
                                (gfloat)(screen_height + 37 - 45 -
                                         TOOLBAR_HEIGHT));
    }
}

/*
 * Because of the way the live windows are implemented, hiding of MutterWindows
 * is accomplished by moving them into a special hidden group; so we have to
 * watch the parent-set signal, and if the parent matches the window group, we
 * make the actor visible, otherwise we hide it.
 *
 * (Should be unnecessary; the IM windows are not getting hidden from within
 * the compositor but from the IM control process, and as such undergo real
 * unmap.)
 */
static void
scim_preview_parent_set_cb (ClutterActor *source,
                            ClutterActor *old_parent,
                            ClutterActor *clone)
{
  MutterPlugin *plugin = moblin_netbook_get_plugin_singleton ();
  MetaScreen   *screen = mutter_plugin_get_screen (plugin);
  ClutterActor *wgroup = mutter_get_window_group_for_screen (screen);
  ClutterActor *parent = clutter_actor_get_parent (source);

  if (parent != wgroup)
    clutter_actor_hide (clone);
  else
    {
      scim_resize_active_panel (plugin, NULL);
      clutter_actor_show (clone);
      clutter_actor_raise_top (clone);
    }
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

  /*
   * The OR test must come first, since GTK_WINDOW_POPUP type windows are
   * both override redirect, but also have a _NET_WM_WINDOW_TYPE set to NORMAL
   */
  if (mutter_window_is_override_redirect (mcw))
    {
      Window       xwin     = mutter_window_get_x_window (mcw);
      MnbToolbar  *toolbar  = MNB_TOOLBAR (priv->toolbar);
      MetaWindow  *mw       = mutter_window_get_meta_window (mcw);
      const gchar *wm_class = meta_window_get_wm_class (mw);
      MnbPanel    *panel;

      /*
       * Handle the case of a IM preview window for a panel.
       *
       * We want to narrow this down before calling XGetProperty().
       */
      if (wm_class && !strcmp (wm_class, "Scim-panel-gtk"))
        {
          if ((panel = (MnbPanel*)mnb_toolbar_get_active_panel (toolbar)))
            {
              ClutterActor *clone   = clutter_clone_new (CLUTTER_ACTOR(mcw));
              MetaScreen   *screen  = mutter_plugin_get_screen (plugin);
              ClutterActor *stage   = mutter_get_stage_for_screen (screen);
              gfloat        x, y;

              /*
               * Let the compositor to finish up mapping of this window.
               */
              mutter_plugin_effect_completed (plugin, mcw,
                                              MUTTER_PLUGIN_MAP);

              /*
               * Now resize the active panel if appropriate.
               */
              scim_resize_active_panel (plugin, panel);

              /*
               * Make a clone and place it on the top of stage.
               */
              clutter_actor_get_position (CLUTTER_ACTOR (mcw), &x, &y);
              clutter_actor_set_position (clone, x, y);

              clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                                           clone);
              clutter_actor_raise_top (clone);

              g_signal_connect (mutter_window_get_texture (mcw),
                                "notify::allocation",
                                G_CALLBACK (scim_preview_texture_allocation_cb),
                                clone);

              g_signal_connect (mw,
                                "raised",
                                G_CALLBACK (scim_preview_raised_cb),
                                clone);

              g_signal_connect (mcw, "notify::allocation",
                                G_CALLBACK (scim_preview_allocation_cb),
                                clone);

              g_signal_connect (mcw, "destroy",
                                G_CALLBACK (scim_preview_destroy_cb),
                                clone);

              g_signal_connect (mutter_window_get_texture (mcw),
                                "queue-redraw",
                                G_CALLBACK (scim_preview_queue_redraw_cb),
                                clone);

              g_signal_connect (mcw, "parent-set",
                                G_CALLBACK (scim_preview_parent_set_cb),
                                clone);
              return;
            }

          /*
           * We know by now that this window requires just a normal OR
           * treatment, so exit here to avoid testing it against panels and
           * system tray windows.
           */
          mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
          return;
        }

      if ((panel = mnb_toolbar_find_panel_for_xid (toolbar, xwin)))
        {
          g_debug ("@@@ setting mutter window for panel %s @@@",
                   mnb_panel_get_name (panel));

          /*
           * Order matters; mnb_panel_show_mutter_window() hides the mcw,
           * and since the compositor call clutter_actor_show() when a map
           * effect completes, we have to first let the compositor to do its
           * thing, and only then impose our will on it.
           */
          mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
          mnb_panel_show_mutter_window (panel, mcw);
        }
#if 1
      /*
       * TODO -- this should go once we convert our applets to the
       * new dbus API.
       */
      else if (mnb_toolbar_is_tray_config_window (toolbar, xwin))
        mnb_toolbar_append_tray_window (toolbar, mcw);
#endif
      else
        mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);

    }


  /*
   * Anything that might be associated with startup notification needs to be
   * handled here; if this list grows, we should just split it further.
   */
  else if (type == META_COMP_WINDOW_NORMAL ||
           type == META_COMP_WINDOW_SPLASHSCREEN ||
           type == META_COMP_WINDOW_DIALOG ||
           type == META_COMP_WINDOW_MODAL_DIALOG)
    {
      ClutterAnimation   *animation;
      EffectCompleteData *data  = g_new0 (EffectCompleteData, 1);
      ActorPrivate       *apriv = get_actor_private (mcw);
      MetaWindow         *mw    = mutter_window_get_meta_window (mcw);

      if (mw)
        {
          gboolean fullscreen, modal = FALSE;

          if (type == META_COMP_WINDOW_MODAL_DIALOG)
            modal = TRUE;

          g_object_get (mw, "fullscreen", &fullscreen, NULL);

          if (fullscreen)
            {
              MetaWorkspace *ws = meta_window_get_workspace (mw);

              if (ws)
                {
                  gint index = meta_workspace_index (ws);

                  fullscreen_app_added (priv, index);
                }
            }

          g_signal_connect (mw, "notify::fullscreen",
                            G_CALLBACK (meta_window_fullcreen_notify_cb),
                            plugin);

          /* Hide toolbar etc in presence of modal dialog */
          if (modal == TRUE)
            clutter_actor_hide (priv->toolbar);
        }

      /*
       * Move application window to a new workspace, if appropriate.
       * (We only move applications, i.e., _NET_WM_WINDOW_TYPE_NORMAL that
       * were start up with SN. We explicitely exclude modal windows and
       * windows that are transient.)
       */
      if (type == META_COMP_WINDOW_NORMAL  &&
          !meta_window_is_modal (mw)       &&
          !meta_window_get_startup_id (mw) &&
          !meta_window_get_transient_for (mw))
        {
          MetaScreen  *screen  = mutter_plugin_get_screen (plugin);
          MetaDisplay *display = meta_screen_get_display (screen);
          guint32      timestamp;

          timestamp = meta_display_get_current_time_roundtrip (display);

          moblin_netbook_move_window_to_its_workspace (plugin,
                                                       mcw,
                                                       timestamp);
        }

      /*
       * Anything that we do not animated exits at this point.
       */
      if (type == META_COMP_WINDOW_DIALOG ||
          type == META_COMP_WINDOW_MODAL_DIALOG)
        {
          mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
          return;
        }

      clutter_actor_move_anchor_point_from_gravity (actor,
                                                    CLUTTER_GRAVITY_CENTER);

      clutter_actor_set_scale (actor, 0.0, 0.0);
      clutter_actor_show (actor);

      animation = clutter_actor_animate (actor, CLUTTER_EASE_OUT_ELASTIC,
                                         MAP_TIMEOUT,
                                         "scale-x", 1.0,
                                         "scale-y", 1.0,
                                         NULL);
      data->plugin = plugin;
      data->actor = actor;
      apriv->tml_map = clutter_animation_get_timeline (animation);

      g_signal_connect (apriv->tml_map,
                        "completed",
                        G_CALLBACK (on_map_effect_complete),
                        data);

      apriv->is_minimized = FALSE;

      g_signal_connect (mw, "workspace-changed",
                        G_CALLBACK (meta_window_workspace_changed_cb),
                        plugin);

      if (type == META_COMP_WINDOW_NORMAL)
        {
          g_signal_connect (mw, "focus",
                            G_CALLBACK (mnb_switcher_meta_window_focus_cb),
                            mnb_toolbar_get_switcher (MNB_TOOLBAR (
                                                              priv->toolbar)));

          g_object_weak_ref (G_OBJECT (mw),
                             mnb_switcher_meta_window_weak_ref_cb,
                             mnb_toolbar_get_switcher (MNB_TOOLBAR (
                                                              priv->toolbar)));
        }
    }
  else
    mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);

}

static void
destroy (MutterPlugin *plugin, MutterWindow *mcw)
{
  MetaCompWindowType          type;
  gint                        workspace;
  MetaWindow                 *meta_win;
  const gchar                *wm_class;
  const gchar                *wm_name;

  type      = mutter_window_get_window_type (mcw);
  workspace = mutter_window_get_workspace (mcw);
  meta_win  = mutter_window_get_meta_window (mcw);
  wm_class  = meta_window_get_wm_class (meta_win);
  wm_name   = meta_window_get_title (meta_win);

  if (type == META_COMP_WINDOW_NORMAL)
    {
      MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
      gboolean                    fullscreen;

      if (wm_class && wm_name &&
          !strcmp (wm_class, "Skype") && strstr (wm_name, "Skype™"))
        {
          gint pid = meta_window_get_pid (meta_win);

          if (pid)
            {
              /*
               * Let the WM finish its dealing with this window first, then
               * just kill the application without mercy ... (this is a
               * punishment for unmapping its window when getting minimized into
               * tray).
               */
              check_for_empty_workspace (plugin, workspace, meta_win);
              mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_DESTROY);

              kill (pid, SIGKILL);
              return;
            }
        }

      g_object_get (meta_win, "fullscreen", &fullscreen, NULL);

      if (fullscreen)
        {
          MetaWorkspace *ws = meta_window_get_workspace (meta_win);

          if (ws)
            {
              gint index = meta_workspace_index (ws);

              fullscreen_app_removed (priv, index);
            }
        }

      /*
       * Disconnect the fullscreen notification handler; strictly speaking
       * this should not be necessary, as the MetaWindow should be going away,
       * but take no chances.
       */
      g_signal_handlers_disconnect_by_func (meta_win,
                                            meta_window_fullcreen_notify_cb,
                                            plugin);

    }

  /*
   * Do not destroy workspace if the closing window is a splash screen.
   * (Sometimes the splash gets destroyed before the application window
   * maps, e.g., Gimp.)
   *
   * NB: This must come before we notify Mutter that the effect completed,
   *     otherwise the destruction of this window will be completed and the
   *     workspace switch effect will crash.
   */
  if (type != META_COMP_WINDOW_SPLASHSCREEN)
    check_for_empty_workspace (plugin, workspace, meta_win);

  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_DESTROY);
}

static void
last_focus_weak_notify_cb (gpointer data, GObject *meta_win)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (data)->priv;

  if ((MetaWindow*)meta_win == priv->last_focused)
    {
      priv->last_focused = NULL;

      /* FIXME */
      g_warning ("just lost the last focused window during grab!\n");
    }
}

void
moblin_netbook_unfocus_stage (MutterPlugin *plugin, guint32 timestamp)
{
  MoblinNetbookPluginPrivate *priv    = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);
  MetaWindow                 *focus;

  if (timestamp == CurrentTime)
    timestamp = clutter_x11_get_current_event_time ();

  /*
   * Work out what we should focus next.
   *
   * First, we tray to get the window from metacity tablist, if that fails
   * fall back on the cached last_focused window.
   */
  focus = meta_display_get_tab_current (display,
                                        META_TAB_LIST_NORMAL,
                                        screen,
                                        NULL);


  if (!focus)
    focus = priv->last_focused;

  if (priv->last_focused)
    {
      g_object_weak_unref (G_OBJECT (priv->last_focused),
                           last_focus_weak_notify_cb, plugin);

      priv->last_focused = NULL;
    }

  priv->holding_focus = FALSE;

  if (focus)
    meta_display_set_input_focus_window (display, focus, FALSE, timestamp);
}

void
moblin_netbook_focus_stage (MutterPlugin *plugin, guint32 timestamp)
{
  MoblinNetbookPluginPrivate *priv    = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);
  Display                    *xdpy    = mutter_plugin_get_xdisplay (plugin);

  if (timestamp == CurrentTime)
    timestamp = clutter_x11_get_current_event_time ();


  /*
   * Map the input blocker window so keystrokes, etc., are not reaching apps.
   */
  if (priv->last_focused)
    g_object_weak_unref (G_OBJECT (priv->last_focused),
                         last_focus_weak_notify_cb, plugin);

  priv->last_focused = meta_display_get_focus_window (display);

  if (priv->last_focused)
    g_object_weak_ref (G_OBJECT (priv->last_focused),
                       last_focus_weak_notify_cb, plugin);

  priv->holding_focus = TRUE;

  XSetInputFocus (xdpy,
                  priv->focus_xwin,
                  RevertToPointerRoot,
                  timestamp);
}

static gboolean
xevent_filter (MutterPlugin *plugin, XEvent *xev)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MnbToolbar                 *toolbar = MNB_TOOLBAR (priv->toolbar);
  NbtkWidget                 *switcher;

  switcher = mnb_toolbar_get_switcher (toolbar);

  if (switcher && mnb_switcher_handle_xevent (MNB_SWITCHER (switcher), xev))
    return TRUE;

  if (xev->type == KeyPress &&
      XKeycodeToKeysym (xev->xkey.display, xev->xkey.keycode, 0) ==
                                                    MOBLIN_PANEL_SHORTCUT_KEY)
    {
      if (!CLUTTER_ACTOR_IS_MAPPED (priv->toolbar))
        {
          /*
           * Set the dont_autohide flag on the toolbar; this stops the panel
           * hiding due to mouse pointer movement until the pointer re-enters
           * the panel (i.e., if the toolbar opens, but the pointer is outside
           * of it, we do not want the toolbar to hide as soon as the user
           * moves the pointer).
           */
          mnb_toolbar_set_dont_autohide (MNB_TOOLBAR (priv->toolbar), TRUE);
          clutter_actor_show (priv->toolbar);
        }
      else
        {
          clutter_actor_hide (priv->toolbar);
        }

      return TRUE;
    }

  if (xev->type == KeyPress || xev->type == KeyRelease)
    {
      MetaScreen   *screen  = mutter_plugin_get_screen (plugin);
      ClutterActor *stage   = mutter_get_stage_for_screen (screen);
      Window        xwin;

      /*
       * We only get key events on the no-focus window, but for clutter we
       * need to pretend they come from the stage window.
       */
      xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

      xev->xany.window = xwin;
    }

  return (clutter_x11_handle_event (xev) != CLUTTER_X11_FILTER_CONTINUE);
}

static void
kill_effect (MutterPlugin *plugin, MutterWindow *mcw, gulong event)
{
  ActorPrivate *apriv;

  if (event & MUTTER_PLUGIN_SWITCH_WORKSPACE)
    {
      /*
       * We never kill the zone switching effect; since the effect does not
       * use the MutterWindows directly, it does not screw up the layout.
       */
      return;
    }

  apriv = get_actor_private (mcw);

  if ((event & MUTTER_PLUGIN_MINIMIZE) && apriv->tml_minimize)
    {
      clutter_timeline_stop (apriv->tml_minimize);
      g_signal_emit_by_name (apriv->tml_minimize, "completed", NULL);
    }

  if ((event & MUTTER_PLUGIN_MAXIMIZE) && apriv->tml_maximize)
    {
      clutter_timeline_stop (apriv->tml_maximize);
      g_signal_emit_by_name (apriv->tml_maximize, "completed", NULL);
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
}

static void
setup_focus_window (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  Window                      xwin;
  XSetWindowAttributes        attr;
  Display                    *xdpy    = mutter_plugin_get_xdisplay (plugin);
  MetaScreen                 *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);
  Atom                        type_atom;

  type_atom = meta_display_get_atom (display,
                                     META_ATOM__NET_WM_WINDOW_TYPE_DOCK);

  attr.event_mask        = KeyPressMask | KeyReleaseMask | FocusChangeMask;
  attr.override_redirect = True;

  xwin = XCreateWindow (xdpy,
                        RootWindow (xdpy,
                                    meta_screen_get_screen_number (screen)),
                        -500, -500, 1, 1, 0,
                        CopyFromParent, InputOnly, CopyFromParent,
                        CWEventMask | CWOverrideRedirect, &attr);

  XChangeProperty (xdpy, xwin,
                   meta_display_get_atom (display,
                                          META_ATOM__NET_WM_WINDOW_TYPE),
                   XA_ATOM, 32, PropModeReplace,
                   (unsigned char *) &type_atom,
                   1);

  XMapWindow (xdpy, xwin);

  priv->focus_xwin = xwin;
}

static void
setup_desktop_background (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen = mutter_plugin_get_screen (plugin);
  gint                        screen_width, screen_height;

  mutter_plugin_query_screen_size (MUTTER_PLUGIN (plugin),
                                   &screen_width, &screen_height);

  /* FIXME: pull image from theme, css ? */
  priv->desktop_tex = clutter_texture_new_from_file
                        (THEMEDIR "/panel/background-tile.png", NULL);

  if (priv->desktop_tex == NULL)
    {
      g_warning ("Failed to load '"
                 THEMEDIR
                 "/panel/background-tile.png', No tiled desktop image");
    }
  else
    {
      ClutterActor *stage = mutter_get_stage_for_screen (screen);

      clutter_actor_set_size (priv->desktop_tex, screen_width, screen_height);

#if !USE_SCALED_BACKGROUND
      g_object_set (priv->desktop_tex,
                    "repeat-x", TRUE,
                    "repeat-y", TRUE,
                    NULL);
#endif
      clutter_actor_set_size (priv->desktop_tex, screen_width, screen_height);
      clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                                   priv->desktop_tex);
      clutter_actor_lower_bottom (priv->desktop_tex);
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



void
moblin_netbook_stash_window_focus (MutterPlugin *plugin, guint32 timestamp)
{
  MoblinNetbookPluginPrivate *priv    = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);
  Display                    *xdpy    = mutter_plugin_get_xdisplay (plugin);
  MetaWindow                 *mutter_last_focused;

  if (timestamp == CurrentTime)
    timestamp = clutter_x11_get_current_event_time ();

  /*
   * Only change the stored last_focused window if Mutter gives us something
   * meaningful to work with, otherwise, we stick wit what we have.
   */
  mutter_last_focused = meta_display_get_focus_window (display);

  if (mutter_last_focused && (mutter_last_focused != priv->last_focused))
    {
      if (priv->last_focused)
        g_object_weak_unref (G_OBJECT (priv->last_focused),
                             last_focus_weak_notify_cb, plugin);

      priv->last_focused = mutter_last_focused;

      g_object_weak_ref (G_OBJECT (mutter_last_focused),
                         last_focus_weak_notify_cb, plugin);
    }

  XSetInputFocus (xdpy,
                  priv->focus_xwin,
                  RevertToPointerRoot,
                  timestamp);
}

void
moblin_netbook_unstash_window_focus (MutterPlugin *plugin, guint32 timestamp)
{
  MoblinNetbookPluginPrivate *priv    = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);
  MetaWindow                 *focus;


  if (timestamp == CurrentTime)
    timestamp = clutter_x11_get_current_event_time ();

  /*
   * Work out what we should focus next.
   *
   * First, we tray to get the window from metacity tablist, if that fails
   * fall back on the cached last_focused window.
   */
  focus = meta_display_get_tab_current (display,
                                        META_TAB_LIST_NORMAL,
                                        screen,
                                        NULL);

  if (!focus)
    focus = priv->last_focused;

  if (priv->last_focused)
    {
      g_object_weak_unref (G_OBJECT (priv->last_focused),
                           last_focus_weak_notify_cb, plugin);

      priv->last_focused = NULL;
    }

  /*
   * If we have something to focus, than do so; otherwise we focus the Mutter
   * no focus window, so that the focus departs from whatever UI element
   * (such as a panel) was holding it.
   */
  if (focus)
    meta_display_set_input_focus_window (display, focus, FALSE, timestamp);
  else
    meta_display_focus_the_no_focus_window (display, screen, timestamp);
}

struct MnbInputRegion
{
  XserverRegion region;
};

MnbInputRegion
moblin_netbook_input_region_push (MutterPlugin *plugin,
                                  gint          x,
                                  gint          y,
                                  guint         width,
                                  guint         height)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MnbInputRegion mir = g_slice_alloc (sizeof (struct MnbInputRegion));
  XRectangle     rect;
  Display       *xdpy = mutter_plugin_get_xdisplay (plugin);

  rect.x       = x;
  rect.y       = y;
  rect.width   = width;
  rect.height  = height;

  mir->region  = XFixesCreateRegion (xdpy, &rect, 1);

  priv->input_region_stack = g_list_append (priv->input_region_stack, mir);

  moblin_netbook_input_region_apply (plugin);

  return mir;
}

void
moblin_netbook_input_region_remove (MutterPlugin *plugin, MnbInputRegion mir)
{
  moblin_netbook_input_region_remove_without_update (plugin, mir);
  moblin_netbook_input_region_apply (plugin);
}

void
moblin_netbook_input_region_remove_without_update (MutterPlugin  *plugin,
                                                   MnbInputRegion mir)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  Display *xdpy = mutter_plugin_get_xdisplay (plugin);

  if (mir->region)
    XFixesDestroyRegion (xdpy, mir->region);

  priv->input_region_stack = g_list_remove (priv->input_region_stack, mir);

  g_slice_free (struct MnbInputRegion, mir);
}

static void
moblin_netbook_input_region_apply (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  Display *xdpy = mutter_plugin_get_xdisplay (plugin);
  GList *l = priv->input_region_stack;
  XserverRegion result;

  if (priv->current_input_region)
    XFixesDestroyRegion (xdpy, priv->current_input_region);

  result = priv->current_input_region = XFixesCreateRegion (xdpy, NULL, 0);

  while (l)
    {
      MnbInputRegion mir = l->data;

      if (mir->region)
        XFixesUnionRegion (xdpy, result, result, mir->region);

      l = l->next;
    }

  mutter_plugin_set_stage_input_region (plugin, result);
}

static gboolean
on_lowlight_button_event (ClutterActor *actor,
                          ClutterEvent *event,
                          gpointer      user_data)
{
  return TRUE;                  /* Simply block events being handled */
}

void
moblin_netbook_set_lowlight (MutterPlugin *plugin, gboolean on)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  static MnbInputRegion input_region;
  static gboolean active = FALSE;

  if (on && !active)
    {
      gint screen_width, screen_height;

      mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

      input_region
        = moblin_netbook_input_region_push (plugin,
                                            0, 0, screen_width, screen_height);

      clutter_actor_show (priv->lowlight);
      active = TRUE;
      mnb_toolbar_set_disabled (MNB_TOOLBAR (priv->toolbar), TRUE);
    }
  else
    {
      if (active)
        {
          clutter_actor_hide (priv->lowlight);
          moblin_netbook_input_region_remove (plugin, input_region);
          active = FALSE;
          mnb_toolbar_set_disabled (MNB_TOOLBAR (priv->toolbar), FALSE);
        }
    }
}

MutterPlugin *
moblin_netbook_get_plugin_singleton (void)
{
  return plugin_singleton;
}

