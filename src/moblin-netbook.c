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
#include "switcher/mnb-switcher.h"
#include "mnb-toolbar.h"
#include "effects/mnb-switch-zones-effect.h"
#include "notifications/mnb-notification-gtk.h"

#include <glib/gi18n.h>
#include <gdk/gdkx.h>

#include <moblin-panel/mpl-panel-common.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gmodule.h>
#include <string.h>
#include <signal.h>
#include <X11/extensions/Xcomposite.h>

#include <compositor-mutter.h>
#include <display.h>
#include <prefs.h>
#include <keybindings.h>

#define MINIMIZE_TIMEOUT            250
#define MAXIMIZE_TIMEOUT            250
#define MAP_TIMEOUT                 350
#define SWITCH_TIMEOUT              400
#define WS_SWITCHER_SLIDE_TIMEOUT   250
#define MYZONE_TIMEOUT              200
#define ACTOR_DATA_KEY "MCCP-moblin-netbook-actor-data"

#define KEY_DIR "/desktop/moblin/background"
#define KEY_BG_FILENAME KEY_DIR "/picture_filename"

static MutterPlugin *plugin_singleton = NULL;

/* callback data for when animations complete */
typedef struct
{
  ClutterActor *actor;
  MutterPlugin *plugin;
} EffectCompleteData;

static void desktop_background_init (MutterPlugin *plugin);
static void setup_focus_window (MutterPlugin *plugin);

static void fullscreen_app_added (MutterPlugin *, MetaWindow *);
static void fullscreen_app_removed (MutterPlugin *, MetaWindow *);
static void meta_window_fullscreen_notify_cb (GObject    *object,
                                              GParamSpec *spec,
                                              gpointer    data);
static void moblin_netbook_toggle_compositor (MutterPlugin *, gboolean on);
static void window_destroyed_cb (MutterWindow *mcw, MutterPlugin *plugin);

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
  mnb_input_manager_destroy ();

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
  MoblinNetbookPluginPrivate  *priv = plugin->priv;
  MnbInputRegion             **region;

  if (notify_actor == priv->notification_urgent)
    region = &priv->notification_urgent_input_region;
  else
    region = &priv->notification_cluster_input_region;

  if (*region != NULL)
    {
      mnb_input_manager_remove_region (*region);
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
          *region = mnb_input_manager_push_region (x, y, width, height,
                                                   FALSE, MNB_INPUT_LAYER_TOP);
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
on_urgent_notify_allocation_cb (ClutterActor *notify_urgent,
                                GParamSpec   *pspec,
                                MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = ((MoblinNetbookPlugin *)plugin)->priv;

  gint   screen_width, screen_height;
  gfloat w, h;

  mutter_plugin_query_screen_size (MUTTER_PLUGIN (plugin),
                                   &screen_width, &screen_height);
  clutter_actor_get_size (priv->notification_urgent, &w, &h);

  clutter_actor_set_position (priv->notification_urgent,
                              (screen_width - (int)w) / 2,
                              (screen_height - (int)h) / 2);
}

static void
moblin_netbook_workarea_changed_foreach (MnbDropDown *panel, gpointer data)
{
  mnb_drop_down_ensure_size (panel);
}

/*
 * If the work area size changes while a panel is present (e.g., a VKB pops up),
 * we need to resize the panel.
 */
static void
moblin_netbook_workarea_changed_cb (MetaScreen *screen, MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  mnb_toolbar_foreach_panel (MNB_TOOLBAR (priv->toolbar),
                             moblin_netbook_workarea_changed_foreach,
                             NULL);
}

static void
moblin_netbook_overlay_key_cb (MetaDisplay *display, MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  if (CLUTTER_ACTOR_IS_MAPPED (priv->notification_urgent))
    {
      /*
       * Ignore the overlay key if we have urgent notifications (MB#6036)
       */
      return;
    }

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
      mnb_toolbar_hide (MNB_TOOLBAR (priv->toolbar));
    }
}

static gboolean
moblin_netbook_fullscreen_apps_present_on_workspace (MutterPlugin *plugin,
                                                     gint          index)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  GList *l;

  l = priv->fullscreen_wins;

  while (l)
    {
      MetaWindow    *m = l->data;
      MetaWorkspace *w;

      if (meta_window_is_on_all_workspaces (m))
        return TRUE;

      w = meta_window_get_workspace (m);

      if (w && index == meta_workspace_index (w))
        return TRUE;

      l = l->next;
    }

  return FALSE;
}

static void
moblin_netbook_workspace_switched_cb (MetaScreen          *screen,
                                      gint                 from,
                                      gint                 to,
                                      MetaMotionDirection  dir,
                                      MutterPlugin        *plugin)
{
  gboolean on;

  on = !moblin_netbook_fullscreen_apps_present_on_workspace (plugin, to);

  moblin_netbook_toggle_compositor (plugin, on);
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
  gfloat         w, h;
  ClutterColor   low_clr = { 0, 0, 0, 0x7f };
  GError        *err = NULL;
  MoblinNetbookNotifyStore *notify_store;

  MetaScreen   *screen  = mutter_plugin_get_screen (MUTTER_PLUGIN (plugin));
  MetaDisplay  *display = meta_screen_get_display (screen);
  ClutterActor *stage   = mutter_get_stage_for_screen (screen);

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

  g_signal_connect (screen,
                    "workareas-changed",
                    G_CALLBACK (moblin_netbook_workarea_changed_cb),
                    plugin);

  g_signal_connect (screen,
                    "workspace-switched",
                    G_CALLBACK (moblin_netbook_workspace_switched_cb),
                    plugin);

  g_signal_connect (display,
                    "overlay-key",
                    G_CALLBACK (moblin_netbook_overlay_key_cb),
                    plugin);

  mutter_plugin_query_screen_size (MUTTER_PLUGIN (plugin),
                                   &screen_width, &screen_height);

  if (mutter_plugin_debug_mode (MUTTER_PLUGIN (plugin)))
    {
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

  mnb_input_manager_create (MUTTER_PLUGIN (plugin));

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

  desktop_background_init (MUTTER_PLUGIN (plugin));

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

  clutter_actor_get_size (priv->notification_urgent, &w, &h);
  clutter_actor_set_position (priv->notification_urgent,
                              (screen_width - (int)w) / 2,
                              (screen_height - (int)h) / 2);

  g_signal_connect (priv->notification_urgent,
                    "notify::allocation",
                    G_CALLBACK (on_urgent_notify_allocation_cb),
                    MUTTER_PLUGIN (plugin));

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
                         NULL);

  clutter_container_add (CLUTTER_CONTAINER (stage),
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

          if (m != mcw &&
              (type == META_COMP_WINDOW_NORMAL) &&
              !meta_window_is_hidden (mw) &&
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

/*
 * Shows the myzone if no applications are running.
 *
 * Always returns FALSE, so that we can pass it directly into g_timeout_add()
 * as a one-of check.
 */
static gboolean
maybe_show_myzone (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen = mutter_plugin_get_screen (plugin);
  gboolean                    no_apps = TRUE;
  GList                      *l;

  /*
   * Check for running applications; we do this by checking if any
   * application-type windows are present.
   */
  l = mutter_get_windows (screen);

  while (l)
    {
      MutterWindow       *m    = l->data;
      MetaCompWindowType  type = mutter_window_get_window_type (m);

      /*
       * Ignore desktop, docs, and panel windows
       *
       * (Panel windows are currently of type META_COMP_WINDOW_OVERRIDE_OTHER)
       */
      if (!(type == META_COMP_WINDOW_DESKTOP        ||
            type == META_COMP_WINDOW_DOCK           ||
            type == META_COMP_WINDOW_OVERRIDE_OTHER))
        {
          /* g_debug ("Found singificant window %s of type %d", */
          /*          mutter_window_get_description (m), type); */

          no_apps = FALSE;
          break;
        }

      l = l->next;
    }

  if (no_apps)
    mnb_toolbar_activate_panel (MNB_TOOLBAR (priv->toolbar), MPL_PANEL_MYZONE);

  return FALSE;
}

static void
check_for_empty_workspace (MutterPlugin *plugin,
                           gint workspace, MetaWindow *ignore,
                           gboolean win_destroyed)
{
  MetaScreen *screen = mutter_plugin_get_screen (plugin);
  gboolean    workspace_empty = TRUE;
  GList      *l;

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
       * We need to check this window is not the window we are too ignore. If
       * the ingore window was not destroyed (i.e., it was moved to a different
       * workspace, we have to ignore its transients in the test, (because when
       * a window is moved to a different workspace, the WM automatically moves
       * its transients too).
       *
       * We skip over windows that are on all workspaces (they are irrelevant
       * for our purposes, and also because their workspace is the active
       * workspace, we can have a bit of a race here if workspace is being
       * removed when the screen active workspace is not yet updated at this
       * point and we we try to translate it to an index, it blows up on us).
       */
      if (mw != ignore &&
          !meta_window_is_on_all_workspaces (mw) &&
          (win_destroyed || !meta_window_is_ancestor_of_transient (ignore, mw)))
        {
          /* g_debug ("querying workspace for [%s]", */
          /*          mutter_window_get_description (m)); */

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
      MetaWorkspace  *active_ws;
      guint32         timestamp;
      gint            next_index = -1;

      timestamp  = clutter_x11_get_current_event_time ();
      current_ws = meta_screen_get_workspace_by_index (screen, workspace);
      active_ws  = meta_screen_get_active_workspace (screen);

      if (active_ws == current_ws)
        {
          /*
           * We need to activate the next workspace before we remove this one,
           * so that the zone switch effect works.
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
        }

      meta_screen_remove_workspace (screen, current_ws, timestamp);
    }

  /*
   * Check if we show the myzone; we do this after a short timeout, so that
   * we do not drop the myzone if the application does something like first
   * pop a root password dialog, and only after the dialog closes, creates its
   * main window (see MB#4766).
   */
  g_timeout_add (MYZONE_TIMEOUT, (GSourceFunc)maybe_show_myzone, plugin);
}

/*
 * Checks for presence of windows of given class the name of which *contains*
 * the wm_name string, skipping the ignore window.
 */
static gboolean
check_for_windows_of_wm_class_and_name (MutterPlugin *plugin,
                                        const gchar  *wm_class,
                                        const gchar  *wm_name,
                                        MutterWindow *ignore)
{
  MetaScreen *screen = mutter_plugin_get_screen (plugin);
  GList      *l;

  if (!wm_class)
    return FALSE;

  l = mutter_get_windows (screen);

  while (l)
    {
      MutterWindow *m = l->data;

      if (m != ignore)
        {
          MetaWindow  *win;
          const gchar *klass;
          const gchar *name;

          win   = mutter_window_get_meta_window (m);
          klass = meta_window_get_wm_class (win);
          name  = meta_window_get_title (win);

          if (klass && !strcmp (wm_class, klass) && strstr (name, wm_name))
            return TRUE;
        }

      l = l->next;
    }

  return FALSE;
}

static void
handle_window_destruction (MutterWindow *mcw, MutterPlugin *plugin)
{
  MetaCompWindowType  type;
  gint                workspace;
  MetaWindow         *meta_win;
  const gchar        *wm_class;
  const gchar        *wm_name;

  type      = mutter_window_get_window_type (mcw);
  workspace = mutter_window_get_workspace (mcw);
  meta_win  = mutter_window_get_meta_window (mcw);
  wm_class  = meta_window_get_wm_class (meta_win);
  wm_name   = meta_window_get_title (meta_win);

  if (type == META_COMP_WINDOW_NORMAL)
    {
      g_signal_handlers_disconnect_by_func (mcw,
                                            window_destroyed_cb,
                                            plugin);

      /*
       * If the closed window is one of the main Skype windows, and no other
       * toplevel skype windows are present, kill the application.
       */
      if (wm_class && wm_name &&
          !strcmp (wm_class, "Skype") && strstr (wm_name, "Skype™") &&
          !check_for_windows_of_wm_class_and_name (plugin,
                                                   "Skype", "Skype™", mcw))
        {
          gint pid = meta_window_get_pid (meta_win);

          if (pid)
            kill (pid, SIGKILL);
        }

      /*
       * Remove the window from the fullscreen list; do this unconditionally,
       * so that under no circumstance we leave a dangling pointer behind.
       */
      fullscreen_app_removed (plugin, meta_win);

      /*
       * Disconnect the fullscreen notification handler; strictly speaking
       * this should not be necessary, as the MetaWindow should be going away,
       * but take no chances.
       */
      g_signal_handlers_disconnect_by_func (meta_win,
                                            meta_window_fullscreen_notify_cb,
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
    check_for_empty_workspace (plugin, workspace, meta_win, TRUE);
}

static void
window_destroyed_cb (MutterWindow *mcw, MutterPlugin *plugin)
{
  handle_window_destruction (mcw, plugin);
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
  MutterPlugin *plugin = MUTTER_PLUGIN (data);

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

  check_for_empty_workspace (plugin, old_workspace, mw, FALSE);
}

static void
fullscreen_app_added (MutterPlugin *plugin, MetaWindow *mw)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gboolean                    compositor_on;

  priv->fullscreen_wins = g_list_prepend (priv->fullscreen_wins, mw);

  compositor_on = !moblin_netbook_fullscreen_apps_present (plugin);
  moblin_netbook_toggle_compositor (plugin, compositor_on);
}

static void
fullscreen_app_removed (MutterPlugin *plugin, MetaWindow *mw)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gboolean                    compositor_on;

  priv->fullscreen_wins = g_list_remove (priv->fullscreen_wins, mw);

  compositor_on = !moblin_netbook_fullscreen_apps_present (plugin);
  moblin_netbook_toggle_compositor (plugin, compositor_on);
}

gboolean
moblin_netbook_fullscreen_apps_present (MutterPlugin *plugin)
{
  MetaScreen *screen = mutter_plugin_get_screen (plugin);
  gint        active;

  active = meta_screen_get_active_workspace_index (screen);

  return moblin_netbook_fullscreen_apps_present_on_workspace (plugin, active);
}

static void
moblin_netbook_detach_mutter_windows (MetaScreen *screen)
{
  GList* l = mutter_get_windows (screen);

  while (l)
    {
      MutterWindow *m = l->data;
      if (m)
        {
          /* we need to repair the window pixmap here */
          mutter_window_detach (m);
        }

      l = l->next;
    }
}

static void
moblin_netbook_toggle_compositor (MutterPlugin *plugin, gboolean on)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen;
  Display                    *xdpy;
  Window                      xroot;
  Window                      overlay;

  if ((priv->compositor_disabled && !on) || (!priv->compositor_disabled && on))
    return;

  screen  = mutter_plugin_get_screen (plugin);
  xdpy    = mutter_plugin_get_xdisplay (plugin);
  xroot   = meta_screen_get_xroot (screen);
  overlay = mutter_get_overlay_window (screen);

  if (!on)
    {
      priv->compositor_disabled = TRUE;

      XCompositeUnredirectSubwindows (xdpy,
                                      xroot,
                                      CompositeRedirectManual);
      XUnmapWindow (xdpy, overlay);
      XSync (xdpy, FALSE);
    }
  else
    {
      priv->compositor_disabled = FALSE;

      /*
       * Hide the gtk notification notifier if present
       */
      mnb_notification_gtk_hide ();

      /*
       * Order matters; mapping the window before redirection seems to be
       * least visually disruptive. The fullscreen application is displayed
       * pretty much immediately at correct size, with correct contents, only
       * there is brief delay before the Moblin background appears, and the
       * window decoration is drawn.
       *
       * In the oposite order, we get a brief period of random content between
       * the fullscreen and normal view of the application.xs
       */
      XMapWindow (xdpy, overlay);
      XCompositeRedirectSubwindows (xdpy,
                                    xroot,
                                    CompositeRedirectManual);
      XSync (xdpy, FALSE);
      moblin_netbook_detach_mutter_windows (screen);
    }
}

static void
meta_window_fullscreen_notify_cb (GObject    *object,
                                  GParamSpec *spec,
                                  gpointer    data)
{
  MetaWindow *mw = META_WINDOW (object);
  gboolean    fullscreen;

  g_object_get (object, "fullscreen", &fullscreen, NULL);

  if (fullscreen)
    fullscreen_app_added (data, mw);
  else
    fullscreen_app_removed (data, mw);
}


/*
 * SCIM functionality
 *
 * The basic problem is that the IM is implemented using X windows for the
 * preview, toolbar, and various other UI elements; this is fine for regular
 * applications, but does not work for UX Shell UI (the UI is always above
 * the windows, so, the IM windows are covered and non-interactive.
 *
 * To work around this, when a window of WM_CLASS Scim-bridge-gtk maps, we
 * create a clone and place it to the top of the stage, and we also punch a
 * hole into the stage input region so the window can get events.
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
      clutter_actor_show (clone);
      clutter_actor_raise_top (clone);
    }
}

/* missing prototype; for some reason frame.h is not installed. */
Window meta_frame_get_xwindow (MetaFrame *frame);

static void
handle_panel_child (MutterPlugin *plugin, MutterWindow *mcw)
{
  MetaWindow   *mw      = mutter_window_get_meta_window (mcw);
  ClutterActor *clone   = clutter_clone_new (CLUTTER_ACTOR(mcw));
  MetaScreen   *screen  = mutter_plugin_get_screen (plugin);
  ClutterActor *stage   = mutter_get_stage_for_screen (screen);
  gfloat        x, y;
  Window        xid;

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

  mnb_input_manager_push_window (mcw,
                                 MNB_INPUT_LAYER_PANEL_TRANSIENTS);

  if (meta_window_get_frame (mw))
    xid = meta_frame_get_xwindow (meta_window_get_frame (mw));
  else
    xid = meta_window_get_xwindow (mw);

  gdk_error_trap_push ();

  XRaiseWindow (GDK_DISPLAY (), xid);

  gdk_flush ();
  gdk_error_trap_pop ();
}

/*
 * Simple map handler: it applies a scale effect which must be reversed on
 * completion).
 */
static void
map (MutterPlugin *plugin, MutterWindow *mcw)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MnbToolbar                 *toolbar = MNB_TOOLBAR (priv->toolbar);
  ClutterActor               *actor = CLUTTER_ACTOR (mcw);
  MetaCompWindowType          type;
  MnbPanel                   *active_panel;
  Window                      xwin;

  active_panel = (MnbPanel*)mnb_toolbar_get_active_panel (toolbar);
  type         = mutter_window_get_window_type (mcw);
  xwin         = mutter_window_get_x_window (mcw);

  if (active_panel && MNB_IS_PANEL (active_panel) &&
      mnb_panel_owns_window (active_panel, mcw))
    {
      mutter_plugin_effect_completed (plugin, mcw,
                                      MUTTER_PLUGIN_MAP);

      handle_panel_child (plugin, mcw);
      return;
    }

  /*
   * The OR test must come before the type test since GTK_WINDOW_POPUP type
   * windows are both override redirect, but also have a _NET_WM_WINDOW_TYPE set
   * to NORMAL
   */
  else if (mutter_window_is_override_redirect (mcw))
    {
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
          if (active_panel)
            {
              /*
               * Let the compositor to finish up mapping of this window.
               */
              mutter_plugin_effect_completed (plugin, mcw,
                                              MUTTER_PLUGIN_MAP);


              handle_panel_child (plugin, mcw);
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
          /*
           * Order matters; mnb_panel_show_mutter_window() hides the mcw,
           * and since the compositor call clutter_actor_show() when a map
           * effect completes, we have to first let the compositor to do its
           * thing, and only then impose our will on it.
           */
          mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
          mnb_panel_show_mutter_window (panel, mcw);
        }
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
      ActorPrivate       *apriv = get_actor_private (mcw);
      MetaWindow         *mw    = mutter_window_get_meta_window (mcw);
      MetaScreen        *screen = mutter_plugin_get_screen (plugin);
      gint               screen_width, screen_height;
      gfloat             actor_width, actor_height;
      gboolean           fullscreen = FALSE;

      if (mw)
        {
          gboolean system_modal = FALSE;

          if (meta_window_is_modal (mw) &&
              !meta_window_get_transient_for (mw))
            {
              system_modal = TRUE;
            }

          g_object_get (mw, "fullscreen", &fullscreen, NULL);

          if (fullscreen)
            {
              MetaWorkspace *ws = meta_window_get_workspace (mw);

              if (ws)
                {
                  fullscreen_app_added (plugin, mw);
                  clutter_actor_hide (CLUTTER_ACTOR (mcw));
                }
            }

          g_signal_connect (mw, "notify::fullscreen",
                            G_CALLBACK (meta_window_fullscreen_notify_cb),
                            plugin);

          /* Hide toolbar etc in presence of modal window */
          if (system_modal == TRUE)
            mnb_toolbar_hide (MNB_TOOLBAR (priv->toolbar));
        }

      if (type == META_COMP_WINDOW_NORMAL)
        {
          g_signal_connect (mcw, "window-destroyed",
                            G_CALLBACK (window_destroyed_cb),
                            plugin);
        }

      /*
       * Move application window to a new workspace, if appropriate.
       * (We only move applications, i.e., _NET_WM_WINDOW_TYPE_NORMAL that
       * were start up with SN. We explicitely exclude modal windows and
       * windows that are transient.)
       */
      if (type == META_COMP_WINDOW_NORMAL  &&
          !meta_window_is_modal (mw)       &&
          meta_window_get_startup_id (mw) &&
          meta_window_get_transient_for_as_xid (mw) == None)
        {
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
          type == META_COMP_WINDOW_MODAL_DIALOG ||
          fullscreen)
        {
          mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
          return;
        }

      apriv->is_minimized = FALSE;

      g_signal_connect (mw, "workspace-changed",
                        G_CALLBACK (meta_window_workspace_changed_cb),
                        plugin);

      /*
       * Only animate windows that are smaller than the screen size
       * (see MB#5273)
       */
      mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);
      clutter_actor_get_size (actor, &actor_width, &actor_height);

      if ((gint)actor_width  < screen_width ||
          (gint)actor_height < screen_height)
        {
          EffectCompleteData *data  = g_new0 (EffectCompleteData, 1);

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
        }
      else
        mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
    }
  else
    mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
}

static void
destroy (MutterPlugin *plugin, MutterWindow *mcw)
{
  handle_window_destruction (mcw, plugin);
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

static void
focus_xwin (MutterPlugin *plugin, guint xid)
{
  MetaDisplay *display;

  display = meta_screen_get_display (mutter_plugin_get_screen (plugin));

  gdk_error_trap_push ();

  XSetInputFocus (meta_display_get_xdisplay (display), xid,
                  RevertToPointerRoot, CurrentTime);

  gdk_flush ();
  gdk_error_trap_pop ();
}

void
moblin_netbook_focus_stage (MutterPlugin *plugin, guint32 timestamp)
{
  MoblinNetbookPluginPrivate *priv    = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);

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

  focus_xwin (plugin, priv->focus_xwin);
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

GdkRegion *mutter_window_get_obscured_region (MutterWindow *cw);
gboolean   mutter_window_effect_in_progress (MutterWindow *self);

/*
 * Based on the occlusion code in MutterWindowGroup
 *
 * Returns the visible region that needs to be painted, or NULL, if the
 * entire area needs to be painted.
 */
GdkRegion *
mnb_get_background_visible_region (MetaScreen *screen)
{
  GList       *l;
  GdkRegion   *visible_region;
  GdkRectangle screen_rect = { 0 };

  l = mutter_get_windows (screen);

  /* Start off with the full screen area (for a multihead setup, we
   * might want to use a more accurate union of the monitors to avoid
   * painting in holes from mismatched monitor sizes. That's just an
   * optimization, however.)
   */
  meta_screen_get_size (screen, &screen_rect.width, &screen_rect.height);
  visible_region = gdk_region_rectangle (&screen_rect);

  for (; l; l = l->next)
    {
      MutterWindow *cw;
      ClutterActor *actor;

      if (!MUTTER_IS_WINDOW (l->data) || !CLUTTER_ACTOR_IS_VISIBLE (l->data))
        continue;

      cw = l->data;
      actor = l->data;

      if (mutter_window_effect_in_progress (cw))
        {
          gdk_region_destroy (visible_region);
          return NULL;
        }

      /*
       * MutterWindowGroup adds a test whether the actor is transformed or not;
       * since we do not transform windows in any way, this was omitted.
       */
      if (clutter_actor_get_paint_opacity (actor) == 0xff)
        {
          GdkRegion *obscured_region;

          if ((obscured_region = mutter_window_get_obscured_region (cw)))
            {
              gfloat x, y;

              /* Temporarily move to the coordinate system of the actor */
              clutter_actor_get_position (actor, &x, &y);
              gdk_region_offset (visible_region, - (gint)x, - (gint)y);

              gdk_region_subtract (visible_region, obscured_region);

              /* Move back to original coord system */
              gdk_region_offset (visible_region, (gint)x, (gint)y);
            }
        }
    }

  return visible_region;
}

/*
 * Based on mutter_shaped_texture_paint()
 *
 * Returns TRUE if the texture was painted; if FALSE, regular path for
 * painting textures should be fallen back on.
 */
static gboolean
mnb_desktop_texture_paint (ClutterActor *actor, MetaScreen *screen)
{
  static CoglHandle material = COGL_INVALID_HANDLE;

  CoglHandle paint_tex;
  guint tex_width, tex_height;
  ClutterActorBox alloc;
  GdkRegion *visible_region;

  visible_region = mnb_get_background_visible_region (screen);

  if (!visible_region)
    return FALSE;

  if (gdk_region_empty (visible_region))
    goto finish_up;

  if (!CLUTTER_ACTOR_IS_REALIZED (actor))
    clutter_actor_realize (actor);

  paint_tex = clutter_texture_get_cogl_texture (CLUTTER_TEXTURE (actor));

  tex_width = cogl_texture_get_width (paint_tex);
  tex_height = cogl_texture_get_height (paint_tex);

  if (tex_width == 0 || tex_height == 0) /* no contents yet */
    goto finish_up;

  if (paint_tex == COGL_INVALID_HANDLE)
    goto finish_up;

  if (material == COGL_INVALID_HANDLE)
    material = cogl_material_new ();

  cogl_material_set_layer (material, 0, paint_tex);

  {
    CoglColor color;
    guchar opacity = clutter_actor_get_paint_opacity (actor);
    cogl_color_set_from_4ub (&color, opacity, opacity, opacity, opacity);
    cogl_material_set_color (material, &color);
  }

  cogl_set_source (material);

  clutter_actor_get_allocation_box (actor, &alloc);

  if (!gdk_region_empty (visible_region))
    {
      GdkRectangle *rects;
      int           n_rects;
      int           i;

      /* Limit to how many separate rectangles we'll draw; beyond this just
       * fall back and draw the whole thing */
#     define MAX_RECTS 16

      /* Would be nice to be able to check the number of rects first */
      gdk_region_get_rectangles (visible_region, &rects, &n_rects);
      if (n_rects > MAX_RECTS)
	{
	  g_free (rects);

          cogl_rectangle (0, 0, alloc.x2 - alloc.x1, alloc.y2 - alloc.y1);
	}
      else
	{
	  float coords[MAX_RECTS * 8];
	  for (i = 0; i < n_rects; i++)
	    {
	      GdkRectangle *rect = &rects[i];

	      coords[i * 8 + 0] = rect->x;
	      coords[i * 8 + 1] = rect->y;
	      coords[i * 8 + 2] = rect->x + rect->width;
	      coords[i * 8 + 3] = rect->y + rect->height;
	      coords[i * 8 + 4] = rect->x / (alloc.x2 - alloc.x1);
	      coords[i * 8 + 5] = rect->y / (alloc.y2 - alloc.y1);
	      coords[i * 8 + 6] = (rect->x + rect->width) / (alloc.x2 - alloc.x1);
	      coords[i * 8 + 7] = (rect->y + rect->height) / (alloc.y2 - alloc.y1);
	    }

	  g_free (rects);

	  cogl_rectangles_with_texture_coords (coords, n_rects);
	}
    }

 finish_up:
  gdk_region_destroy (visible_region);
  return TRUE;
}

/*
 * Avoid painting the desktop background if it is completely occluded.
 */
static void
desktop_background_paint (ClutterActor *background, MutterPlugin *plugin)
{
  MetaScreen *screen;

  /*
   * If we are painting a clone, do nothing letting the ClutterTexture paint
   * kick in.
   */
  if (clutter_actor_is_in_clone_paint (background))
    return;

  /*
   * Don't paint desktop background if fullscreen application is present.
   */
  if (moblin_netbook_fullscreen_apps_present (plugin))
    goto finish_up;

  /*
   * Try to paint only parts of the desktop background
   */
  screen = mutter_plugin_get_screen (plugin);
  if (!mnb_desktop_texture_paint (background, screen))
    {
      /*
       * We have not painted the texture, so we leave without stoping
       * emission of the signal.
       */
      return;
    }

 finish_up:
  g_signal_stop_emission_by_name (background, "paint");
}

static void
setup_desktop_background (MutterPlugin *plugin, const gchar *filename)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen = mutter_plugin_get_screen (plugin);
  gint                        screen_width, screen_height;
  gboolean                    new_texture = FALSE;

  mutter_plugin_query_screen_size (MUTTER_PLUGIN (plugin),
                                   &screen_width, &screen_height);

  g_assert (filename);

  if (!priv->desktop_tex)
    {
      new_texture = TRUE;
      priv->desktop_tex = clutter_texture_new_from_file (filename, NULL);
    }
  else
    clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->desktop_tex),
                                   filename, NULL);

  if (priv->desktop_tex == NULL)
    {
      g_warning ("Failed to load '%s', No tiled desktop image", filename);
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

      if (new_texture)
        {
          clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                                       priv->desktop_tex);
          clutter_actor_lower_bottom (priv->desktop_tex);

          g_signal_connect (priv->desktop_tex, "paint",
                            G_CALLBACK (desktop_background_paint),
                            plugin);
        }
    }
}

static void
desktop_filename_changed_cb (GConfClient *client,
                             guint        cnxn_id,
                             GConfEntry  *entry,
                             gpointer     data)
{
  MutterPlugin *plugin = MUTTER_PLUGIN (data);
  const gchar  *filename = NULL;
  GConfValue   *value;

  value = gconf_entry_get_value (entry);

  if (value)
    filename = gconf_value_get_string (value);

  if (!filename || !*filename)
    filename = THEMEDIR "/panel/background-tile.png";

  setup_desktop_background (plugin, filename);
}

static void
desktop_background_init (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  GError *error = NULL;

  priv->gconf_client = gconf_client_get_default ();

  gconf_client_add_dir (priv->gconf_client,
                        KEY_DIR,
                        GCONF_CLIENT_PRELOAD_NONE,
                        &error);

  if (error)
    {
      g_warning (G_STRLOC ": Error when adding directory for notification: %s",
                 error->message);
      g_clear_error (&error);
    }

  gconf_client_notify_add (priv->gconf_client,
                           KEY_BG_FILENAME,
                           desktop_filename_changed_cb,
                           plugin,
                           NULL,
                           &error);

  if (error)
    {
      g_warning (G_STRLOC ": Error when adding key for notification: %s",
                 error->message);
      g_clear_error (&error);
    }

  /*
   * Read the background via our notify func
   */
  gconf_client_notify (priv->gconf_client, KEY_BG_FILENAME);
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

  focus_xwin (plugin, priv->focus_xwin);
}

void
moblin_netbook_unstash_window_focus (MutterPlugin *plugin, guint32 timestamp)
{
  MoblinNetbookPluginPrivate *priv    = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);
  MetaWindow                 *focus;
  NbtkWidget                 *panel;

  /*
   * First check if a panel is active; if it's an OOP panel, we focus the panel,
   * if it's the Switcher, we unstash normally.
   */
  panel = mnb_toolbar_get_active_panel (MNB_TOOLBAR (priv->toolbar));

  if (panel)
    {
      if (MNB_IS_PANEL (panel))
        {
          mnb_panel_focus ((MnbPanel*)panel);
          return;
        }
    }

  if (timestamp == CurrentTime)
    timestamp = meta_display_get_current_time_roundtrip (display);

  /*
   * Work out what we should focus next.
   *
   * First, we try to get the window from metacity tablist, if that fails
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
  static MnbInputRegion *input_region = NULL;
  static gboolean        active       = FALSE;

  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  if (on && !active)
    {
      gint          screen_width, screen_height;
      MetaScreen   *screen  = mutter_plugin_get_screen (plugin);
      ClutterActor *stage   = mutter_get_stage_for_screen (screen);
      Window        xwin;
      guint32       timestamp;

      xwin      = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));
      timestamp = clutter_x11_get_current_event_time ();

      mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

      input_region
        = mnb_input_manager_push_region (0, 0, screen_width, screen_height,
                                         FALSE, MNB_INPUT_LAYER_TOP);

      /*
       * Enusre correct stacking position
       */
      clutter_actor_lower (priv->lowlight, priv->notification_urgent);
      clutter_actor_show (priv->lowlight);
      active = TRUE;
      mutter_plugin_begin_modal (plugin, xwin, None,
                                 META_MODAL_POINTER_ALREADY_GRABBED,
                                 timestamp);

      mnb_toolbar_set_disabled (MNB_TOOLBAR (priv->toolbar), TRUE);
    }
  else
    {
      if (active)
        {
          guint32 timestamp;

          timestamp = clutter_x11_get_current_event_time ();

          clutter_actor_hide (priv->lowlight);
          mnb_input_manager_remove_region (input_region);
          input_region = NULL;
          active = FALSE;
          mnb_toolbar_set_disabled (MNB_TOOLBAR (priv->toolbar), FALSE);
          mutter_plugin_end_modal (plugin, timestamp);
        }
    }
}

MutterPlugin *
moblin_netbook_get_plugin_singleton (void)
{
  return plugin_singleton;
}

/*
 * Returns TRUE if a modal window is present on given workspace; if workspace
 * is < 0, returns TRUE if any modal windows are present on any workspace.
 */
gboolean
moblin_netbook_modal_windows_present (MutterPlugin *plugin, gint workspace)
{
  MetaScreen *screen = mutter_plugin_get_screen (plugin);
  GList      *l      = mutter_get_windows (screen);

  while (l)
    {
      MutterWindow *m = l->data;
      MetaWindow   *w = mutter_window_get_meta_window (m);

      /*
       * Working out the workspace index in Mutter requires examining a list,
       * so do the modality test first.
       */
      if (meta_window_is_modal (w))
        {
          gint s = mutter_window_get_workspace (m);

          if (s < 0 || s == workspace)
            return TRUE;
        }

      l = l->next;
     }

  return FALSE;
}

gboolean
moblin_netbook_compositor_disabled (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  return priv->compositor_disabled;
}
