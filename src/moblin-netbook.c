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
#include "moblin-netbook-chooser.h"
#include "moblin-netbook-panel.h"
#include "mnb-drop-down.h"
#include "mnb-switcher.h"

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gmodule.h>
#include <string.h>

#include <compositor-mutter.h>
#include <display.h>
#include <prefs.h>
#include <keybindings.h>

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


/* callback data for when animations complete */
typedef struct
{
  ClutterActor *actor;
  MutterPlugin *plugin;
} EffectCompleteData;

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

static void moblin_netbook_input_region_apply (MutterPlugin *plugin);

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
    {
      XFixesDestroyRegion (xdpy, priv->screen_region);
      priv->screen_region = None;
    }

  if (priv->panel_trigger_region)
    {
      XFixesDestroyRegion (xdpy, priv->panel_trigger_region);
      priv->panel_trigger_region = None;
    }

  if (priv->panel_trigger_region2)
    {
      XFixesDestroyRegion (xdpy, priv->panel_trigger_region2);
      priv->panel_trigger_region2 = None;
    }

  if (priv->current_input_region)
    {
      XFixesDestroyRegion (xdpy, priv->current_input_region);
      priv->current_input_region = None;
    }

  G_OBJECT_CLASS (moblin_netbook_plugin_parent_class)->dispose (object);
}

static void
moblin_netbook_plugin_finalize (GObject *object)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (object)->priv;

  g_list_free (priv->global_tab_list);

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

/*
 * Based on do_choose_window() in metacity keybinding.c
 *
 * This is the machinery we need for handling Alt+Tab
 *
 * To start with, Metacity has a passive grab on this key, so we hook up a
 * custom handler to its keybingins.
 *
 * Once we get the initial Alt+Tab, we establish a global key grab (we use
 * metacity API for this, as we need the regular bindings to be correctly
 * restored when we are finished). When the alt key is released, we
 * release the grab.
 */
static gboolean
alt_still_down (MetaDisplay *display, MetaScreen *screen, Window xwin,
                guint entire_binding_mask)
{
  gint        x, y, root_x, root_y, i;
  Window      root, child;
  guint       mask, primary_modifier = 0;
  Display    *xdpy = meta_display_get_xdisplay (display);
  guint       masks[] = { Mod5Mask, Mod4Mask, Mod3Mask,
                          Mod2Mask, Mod1Mask, ControlMask,
                          ShiftMask, LockMask };

  i = 0;
  while (i < (int) G_N_ELEMENTS (masks))
    {
      if (entire_binding_mask & masks[i])
        {
          primary_modifier = masks[i];
          break;
        }

      ++i;
    }

  XQueryPointer (xdpy,
                 xwin, /* some random window */
                 &root, &child,
                 &root_x, &root_y,
                 &x, &y,
                 &mask);

  if ((mask & primary_modifier) == 0)
    return FALSE;
  else
    return TRUE;
}

/*
 * Helper function for metacity_alt_tab_key_handler().
 *
 * The advance parameter indicates whether if the grab succeeds the switcher
 * selection should be advanced.
 */
static void
try_alt_tab_grab (MutterPlugin *plugin,
                  gulong        mask,
                  guint         timestamp,
                  gboolean      backward,
                  gboolean      advance)
{
  MoblinNetbookPluginPrivate *priv     = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen   = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display  = meta_screen_get_display (screen);
  MnbSwitcher                *switcher = MNB_SWITCHER (priv->switcher);
  MetaWindow                 *next     = NULL;
  MetaWindow                 *current  = NULL;

  current = meta_display_get_tab_current (display,
                                          META_TAB_LIST_NORMAL,
                                          screen,
                                          NULL);

  next = mnb_switcher_get_next_window (switcher, NULL, backward);

  /*
   * If we cannot get a window from the switcher (i.e., the switcher is not
   * visible), get the next suitable window from the global tab list.
   */
  if (!next && priv->global_tab_list)
    {
      GList *l;
      MetaWindow *focused = NULL;

      l = priv->global_tab_list;

      while (l)
        {
          MetaWindow *mw = l->data;
          gboolean    sticky;

          sticky = meta_window_is_on_all_workspaces (mw);

          if (sticky)
            {
              /*
               * The loop runs forward when looking for the focused window and
               * when looking for the next window in forward direction; when
               * looking for the next window in backward direction, runs, ehm,
               * backward.
               */
              if (focused && backward)
                l = l->prev;
              else
                l = l->next;
              continue;
            }

          if (!focused)
            {
              if (meta_window_has_focus (mw))
                focused = mw;
            }
          else
            {
              next = mw;
              break;
            }

          /*
           * The loop runs forward when looking for the focused window and
           * when looking for the next window in forward direction; when
           * looking for the next window in backward direction, runs, ehm,
           * backward.
           */
          if (focused && backward)
            l = l->prev;
          else
            l = l->next;
        }

      /*
       * If all fails, fall back at the start/end of the list.
       */
      if (!next && priv->global_tab_list)
        {
          if (backward)
            next = META_WINDOW (priv->global_tab_list->data);
          else
            next = META_WINDOW (g_list_last (priv->global_tab_list)->data);
        }
    }

  /*
   * If we still do not have the next window, or the one we got so far matches
   * the current window, we fall back onto metacity's focus list and try to
   * switch to that.
   */
  if (current && (!next || (advance  && (next == current))))
    {
      MetaWorkspace *ws = meta_window_get_workspace (current);

      next = meta_display_get_tab_next (display,
                                        META_TAB_LIST_NORMAL,
                                        screen,
                                        ws,
                                        current,
                                        backward);
    }

  if (!next || (advance && (next == current)))
    return;


  /*
   * For some reaon, XGrabKeyboard() does not like real timestamps, or
   * we are getting rubish out of clutter ... using CurrentTime here makes it
   * work.
   */
  if (meta_display_begin_grab_op (display,
                                  screen,
                                  NULL,
                                  META_GRAB_OP_KEYBOARD_TABBING_NORMAL,
                                  FALSE,
                                  FALSE,
                                  0,
                                  mask,
                                  timestamp,
                                  0, 0))
    {
      ClutterActor               *stage = mutter_get_stage_for_screen (screen);
      Window                      xwin;

      xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

      priv->in_alt_grab = TRUE;

      if (!alt_still_down (display, screen, xwin, mask))
        {
          MetaWorkspace *workspace;
          MetaWorkspace *active_workspace;

          meta_display_end_grab_op (display, timestamp);
          priv->in_alt_grab = FALSE;

          workspace        = meta_window_get_workspace (next);
          active_workspace = meta_screen_get_active_workspace (screen);

          clutter_actor_hide (priv->switcher);
          hide_panel (plugin);

          if (!active_workspace || (active_workspace == workspace))
            {
              meta_window_activate_with_workspace (next,
                                                   timestamp,
                                                   workspace);
            }
          else
            {
              meta_workspace_activate_with_focus (workspace,
                                                  next,
                                                  timestamp);
            }
        }
      else
        {
          if (advance)
            mnb_switcher_select_window (switcher, next);
          else if (current)
            mnb_switcher_select_window (switcher, current);
        }
    }
}

static void
handle_alt_tab (MetaDisplay    *display,
                MetaScreen     *screen,
                MetaWindow     *event_window,
                XEvent         *event,
                MetaKeyBinding *binding,
                MutterPlugin   *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gboolean                    backward = FALSE;
  MetaWindow                 *next;
  MetaWorkspace              *workspace;
  MnbSwitcher                *switcher = MNB_SWITCHER (priv->switcher);
  guint32                     timestamp;

  if (event->type != KeyPress)
    return;

  timestamp = meta_display_get_current_time_roundtrip (display);

#if 0
  printf ("got key event (%d) for keycode %d on window 0x%x, sub 0x%x, state %d\n",
          event->type,
          event->xkey.keycode,
          (guint) event->xkey.window,
          (guint) event->xkey.subwindow,
          event->xkey.state);
#endif

  /* reverse direction if shift is down */
  if (event->xkey.state & ShiftMask)
    backward = !backward;

  workspace = meta_screen_get_active_workspace (screen);

  if (priv->in_alt_grab)
    {
      MetaWindow *selected = mnb_switcher_get_selection (switcher);

      next = mnb_switcher_get_next_window (switcher, selected, backward);

      if (!next)
        {
          g_warning ("No idea what the next selected window should be.\n");
          return;
        }

      mnb_switcher_select_window (MNB_SWITCHER (priv->switcher), next);
      return;
    }

  try_alt_tab_grab (plugin, binding->mask, timestamp, backward, FALSE);
}

struct alt_tab_show_complete_data
{
  MutterPlugin   *plugin;
  MetaDisplay    *display;
  MetaScreen     *screen;
  MetaWindow     *window;
  MetaKeyBinding *binding;
  XEvent          xevent;
  gboolean        show_panel;
};

static void
alt_tab_switcher_show_completed_cb (ClutterActor *switcher, gpointer data)
{
  struct alt_tab_show_complete_data *alt_data = data;

  handle_alt_tab (alt_data->display, alt_data->screen, alt_data->window,
                  &alt_data->xevent, alt_data->binding, alt_data->plugin);

  /*
   * This is a one-off, disconnect ourselves.
   */
  g_signal_handlers_disconnect_by_func (switcher,
                                        alt_tab_switcher_show_completed_cb,
                                        data);

  g_free (data);
}

static gboolean
alt_tab_timeout_cb (gpointer data)
{
  struct alt_tab_show_complete_data *alt_data = data;
  MoblinNetbookPluginPrivate        *priv;
  ClutterActor                      *stage;
  Window                             xwin;

  priv  = MOBLIN_NETBOOK_PLUGIN (alt_data->plugin)->priv;
  stage = mutter_get_stage_for_screen (alt_data->screen);
  xwin  = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

  /*
   * Check wether the Alt key is still down; if so, show the Switcher, and
   * wait for the show-completed signal to process the Alt+Tab.
   */
  if (alt_still_down (alt_data->display, alt_data->screen, xwin, Mod1Mask))
    {
      g_signal_connect (priv->switcher, "show-completed",
                        G_CALLBACK (alt_tab_switcher_show_completed_cb),
                        alt_data);

      if (alt_data->show_panel)
        show_panel_and_control (alt_data->plugin, MNBK_CONTROL_SPACES);
      else
        clutter_actor_show (priv->switcher);
    }
  else
    {
      gboolean backward  = FALSE;

      if (alt_data->xevent.xkey.state & ShiftMask)
        backward = !backward;

      try_alt_tab_grab (alt_data->plugin, alt_data->binding->mask,
                        alt_data->xevent.xkey.time, backward, TRUE);
      g_free (data);
    }

  /* One off */
  return FALSE;
}

/*
 * The handler for Alt+Tab that we register with metacity.
 */
static void
metacity_alt_tab_key_handler (MetaDisplay    *display,
                              MetaScreen     *screen,
                              MetaWindow     *window,
                              XEvent         *event,
                              MetaKeyBinding *binding,
                              gpointer        data)
{
  MutterPlugin               *plugin = MUTTER_PLUGIN (data);
  MoblinNetbookPluginPrivate *priv   = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  if (!CLUTTER_ACTOR_IS_VISIBLE (priv->switcher))
    {
      struct alt_tab_show_complete_data *alt_data;

      /*
       * If the switcher is not visible we want to show it; this is, however,
       * complicated by several factors:
       *
       *  a) If the panel is visible, we have to show the panel first. In this
       *     case, the Panel slides out, when the effect finishes, the Switcher
       *     slides from underneath -- clutter_actor_show() is only called on
       *     the switcher when the Panel effect completes, and as the contents
       *     of the Switcher are being built in the _show() virtual, we do not
       *     have those until the effects are all over. We need the switcher
       *     contents initialized before we can start the actual processing of
       *     the Alt+Tab key, so we need to wait for the "show-completed" signal
       *
       *  b) If the user just hits and immediately releases Alt+Tab, we need to
       *     avoid initiating the effects alltogether, otherwise we just get
       *     bit of a flicker as the Switcher starts coming out and immediately
       *     disappears.
       *
       *  So, instead of showing the switcher, we install a timeout to introduce
       *  a short delay, so we can test whether the Alt key is still down. We
       *  then handle the actual show from the timeout.
       */
      alt_data = g_new0 (struct alt_tab_show_complete_data, 1);
      alt_data->display = display;
      alt_data->screen  = screen;
      alt_data->plugin  = plugin;
      alt_data->binding = binding;

      memcpy (&alt_data->xevent, event, sizeof (XEvent));

      if (!CLUTTER_ACTOR_IS_VISIBLE (priv->panel))
        alt_data->show_panel = TRUE;

      g_timeout_add (100, alt_tab_timeout_cb, alt_data);
      return;
    }

  handle_alt_tab (display, screen, window, event, binding, plugin);
}

/*
 * Metacity key handler for default Metacity bindings we want disabled.
 *
 * (This is necessary for keybidings that are related to the Alt+Tab shortcut.
 * In metacity these all use the src/ui/tabpopup.c object, which we have
 * disabled, so we need to take over all of those.)
 */
static void
metacity_nop_key_handler (MetaDisplay    *display,
                          MetaScreen     *screen,
                          MetaWindow     *window,
                          XEvent         *event,
                          MetaKeyBinding *binding,
                          gpointer        data)
{
}

static void
sync_notification_input_region_cb (ClutterActor        *notify_cluster,
                                   MoblinNetbookPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv   = plugin->priv;

  if (priv->notification_input_region != NULL)
    {
      moblin_netbook_input_region_remove (MUTTER_PLUGIN(plugin),
                                          priv->notification_input_region);
      priv->notification_input_region = NULL;
    }

  if (CLUTTER_ACTOR_IS_VISIBLE (notify_cluster))
    {
      gint x,y;
      guint width,height;

      clutter_actor_get_transformed_position (notify_cluster, &x, &y);
      clutter_actor_get_transformed_size (notify_cluster, &width, &height);


      if (width != 0 && height != 0)
        {
          priv->notification_input_region
            = moblin_netbook_input_region_push (MUTTER_PLUGIN(plugin),
                                                x, y, width, height,FALSE);
        }
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

  priv->panel_trigger_region = region;

  rect[0].height += 5;

  region = XFixesCreateRegion (xdpy, &rect[0], 1);

  priv->panel_trigger_region2 = region;

  rect[0].height = screen_height;

  region = XFixesCreateRegion (xdpy, &rect[0], 1);

  priv->screen_region = region;

  /*
   * Create the current_input region; we start with empty, and it gets filled
   * with the initial enable_stage() call.
   */
  priv->current_input_region = XFixesCreateRegion (xdpy, NULL, 0);

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

  overlay = mutter_plugin_get_overlay_group (MUTTER_PLUGIN (plugin));

  lowlight = clutter_rectangle_new_with_color (&low_clr);
  priv->lowlight = lowlight;
  clutter_actor_set_size (lowlight, screen_width, screen_height);

  /*
   * This also creates the launcher.
   */
  panel = priv->panel = make_panel (MUTTER_PLUGIN (plugin), screen_width);
  clutter_actor_realize (priv->panel_shadow);

  clutter_container_add (CLUTTER_CONTAINER (overlay), lowlight, panel, NULL);
  clutter_actor_hide (lowlight);
  clutter_actor_show (priv->mzone_grid);
  priv->panel_wait_for_pointer = TRUE;

  /*
   * Set up the stage even processing
   */
  enable_stage (MUTTER_PLUGIN (plugin), CurrentTime);

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

  moblin_netbook_sn_setup (MUTTER_PLUGIN (plugin));

  // moblin_netbook_notify_init (MUTTER_PLUGIN (plugin));

  /* Notifications */

  priv->notification_cluster = mnb_notification_cluster_new ();

  clutter_container_add (CLUTTER_CONTAINER (overlay),
                         priv->notification_cluster, NULL);

  clutter_actor_set_anchor_point_from_gravity (priv->notification_cluster,
                                               CLUTTER_GRAVITY_SOUTH_EAST);

  clutter_actor_set_position (priv->notification_cluster,
                              screen_width,
                              screen_height);

  g_signal_connect (priv->notification_cluster,
                    "sync-input-region",
                    G_CALLBACK (sync_notification_input_region_cb),
                    MUTTER_PLUGIN (plugin));


#if 0
  {
    ClutterActor *foo;

    foo = clutter_rectangle_new ();
    clutter_actor_set_size (foo, 200, 200);

    clutter_container_add (CLUTTER_CONTAINER (overlay),
                           foo, NULL);

    clutter_actor_set_anchor_point_from_gravity (foo,
                                                 CLUTTER_GRAVITY_SOUTH_EAST);

    clutter_actor_set_position (foo,
                                screen_width - 400,
                                screen_height - 400);
  }
#endif

  /* Keys */

  meta_prefs_override_no_tab_popup (TRUE);

  /*
   * Install our custom Alt+Tab handler.
   */
  meta_keybindings_set_custom_handler ("switch_windows",
                                       metacity_alt_tab_key_handler,
                                       plugin, NULL);
  meta_keybindings_set_custom_handler ("switch_windows_backward",
                                       metacity_alt_tab_key_handler,
                                       plugin, NULL);

  /*
   * Install NOP handler for shortcuts that are related to Alt+Tab.
   */
  meta_keybindings_set_custom_handler ("switch_group",
                                       metacity_nop_key_handler,
                                       plugin, NULL);
  meta_keybindings_set_custom_handler ("switch_group_backward",
                                       metacity_nop_key_handler,
                                       plugin, NULL);
  meta_keybindings_set_custom_handler ("switch_group_backward",
                                       metacity_nop_key_handler,
                                       plugin, NULL);
  meta_keybindings_set_custom_handler ("switch_panels",
                                       metacity_alt_tab_key_handler,
                                       plugin, NULL);
  meta_keybindings_set_custom_handler ("switch_panels_backward",
                                       metacity_alt_tab_key_handler,
                                       plugin, NULL);
  meta_keybindings_set_custom_handler ("cycle_group",
                                       metacity_alt_tab_key_handler,
                                       plugin, NULL);
  meta_keybindings_set_custom_handler ("cycle_group_backward",
                                       metacity_alt_tab_key_handler,
                                       plugin, NULL);
  meta_keybindings_set_custom_handler ("cycle_windows",
                                       metacity_alt_tab_key_handler,
                                       plugin, NULL);
  meta_keybindings_set_custom_handler ("cycle_windows_backward",
                                       metacity_alt_tab_key_handler,
                                       plugin, NULL);
  meta_keybindings_set_custom_handler ("cycle_panels",
                                       metacity_alt_tab_key_handler,
                                       plugin, NULL);
  meta_keybindings_set_custom_handler ("cycle_panels_backward",
                                       metacity_alt_tab_key_handler,
                                       plugin, NULL);

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
  ClutterColor       col = { 0xff, 0xff, 0xff, 0xff };
  CoglHandle         cogl_texture;
  float              t_w, t_h;
  guint              tex_width, tex_height;
  guint              w, h;

  clutter_actor_get_size (priv->parallax_tex, &w, &h);

  cogl_translate (priv->parallax_paint_offset - (gint)w/4, 0 , 0);

  cogl_texture
       = clutter_texture_get_cogl_texture (CLUTTER_TEXTURE(priv->parallax_tex));

  if (cogl_texture == COGL_INVALID_HANDLE)
    return;

  col.alpha = clutter_actor_get_paint_opacity (actor);
  cogl_set_source_color4ub (col.red,
                            col.green,
                            col.blue,
                            col.alpha);

  tex_width = cogl_texture_get_width (cogl_texture);
  tex_height = cogl_texture_get_height (cogl_texture);

  t_w = (float) w / tex_width;
  t_h = (float) h / tex_height;

  /* Parent paint translated us into position */
  cogl_set_source_texture (cogl_texture);
  cogl_rectangle_with_texture_coords (0, 0,
                                      w, h,
                                      0, 0,
                                      t_w, t_h);

  g_signal_stop_emission_by_name (actor, "paint");
}

struct ws_switch_data
{
  GList **actors;
  MutterPlugin *plugin;
};

static void
on_switch_workspace_effect_complete (ClutterTimeline *timeline, gpointer data)
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

  moblin_netbook_sn_finalize (plugin);

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
  gint           to_x, to_y;
  gint           para_dir = 2;
  ClutterColor   white = { 0xff, 0xff, 0xff, 0xff };
  ClutterColor   black = { 0x33, 0x33, 0x33, 0xff };
  gint           screen_width;
  gint           screen_height;
  guint		 indicator_width, indicator_height;
  MetaScreen    *screen = mutter_plugin_get_screen (plugin);
  struct parallax_data *parallax_data = g_new (struct parallax_data, 1);
  struct ws_switch_data *switch_data = g_new (struct ws_switch_data, 1);
  ClutterAnimation *animation;

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

  label = clutter_text_new ();
  clutter_text_set_font_name (CLUTTER_TEXT (label), "Sans Bold 148");
  clutter_text_set_color (CLUTTER_TEXT (label), &black);
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
      clutter_text_set_text (CLUTTER_TEXT (label), "\342\206\221");

      to_x = 0;
      to_y = -screen_height;
      break;

    case META_MOTION_DOWN:
      clutter_text_set_text (CLUTTER_TEXT (label), "\342\206\223");

      to_x = 0;
      to_y = -screen_height;
      break;

    case META_MOTION_LEFT:
      clutter_text_set_text (CLUTTER_TEXT (label), "\342\206\220");

      to_x = -screen_width * -1;
      to_y = 0;

      para_dir = -2;
      break;

    case META_MOTION_RIGHT:
      clutter_text_set_text (CLUTTER_TEXT (label), "\342\206\222");

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
  animation = clutter_actor_animate (workspace_slider1,
                                     CLUTTER_LINEAR,
                                     WS_SWITCHER_SLIDE_TIMEOUT,
                                     "x", 0,
                                     "y", 0,
                                     NULL);

  ppriv->tml_switch_workspace1 = clutter_animation_get_timeline (animation);

  g_signal_connect (clutter_animation_get_timeline (animation),
                    "completed",
                    G_CALLBACK (on_switch_workspace_effect_complete),
                    switch_data);

  /* coming from */
  animation = clutter_actor_animate (workspace_slider0,
                                     CLUTTER_LINEAR,
                                     WS_SWITCHER_SLIDE_TIMEOUT,
                                     "x", to_x,
                                     "y", to_y,
                                     NULL);

  ppriv->tml_switch_workspace0 = clutter_animation_get_timeline (animation);

  /* arrow */
  clutter_actor_animate (indicator_group,
                         CLUTTER_LINEAR,
                         WS_SWITCHER_SLIDE_TIMEOUT,
                         "opacity", 0,
                         NULL);

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
  gint     anchor_x = 0;
  gint     anchor_y = 0;

  type = mutter_window_get_window_type (mcw);

  if (type == META_COMP_WINDOW_NORMAL)
    {
      ActorPrivate *apriv = get_actor_private (mcw);
      ClutterAnimation *animation;
      EffectCompleteData *data = g_new0 (EffectCompleteData, 1);
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
on_config_actor_destroy (ClutterActor *actor, gpointer data)
{
  ClutterActor *background = data;
  ClutterActor *parent = clutter_actor_get_parent (background);

  if (CLUTTER_IS_CONTAINER (parent))
    clutter_container_remove_actor (CLUTTER_CONTAINER (parent), background);
  else
    clutter_actor_unparent (background);
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
on_config_actor_hide_completed_cb (ClutterActor *actor, gpointer data)
{
  struct config_hide_data *hide_data = data;

  shell_tray_manager_close_config_window (hide_data->plugin->priv->tray_manager,
                                          hide_data->config_xwin);
}

static void
on_config_actor_hide_begin_cb (ClutterActor *actor, gpointer data)
{
  struct config_hide_data *hide_data = data;

  shell_tray_manager_hide_config_window (hide_data->plugin->priv->tray_manager,
                                         hide_data->config_xwin);
}

static void
check_for_empty_workspace (MutterPlugin *plugin,
                           gint workspace, MetaWindow *ignore)
{
  MetaScreen *screen = mutter_plugin_get_screen (plugin);
  gboolean    workspace_empty = TRUE;
  GList      *l;

  l = mutter_get_windows (screen);
  while (l)
    {
      MutterWindow *m = l->data;
      MetaWindow   *mw = mutter_window_get_meta_window (m);

      if (mw != ignore)
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
      guint32        timestamp;

      timestamp = clutter_x11_get_current_event_time ();

      mws = meta_screen_get_workspace_by_index (screen, workspace);

      meta_screen_remove_workspace (screen, mws, timestamp);
    }
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

  check_for_empty_workspace (plugin, old_workspace, mw);
}

/*
 * The following two functions are used to maintain the global tab list.
 * When a window is focused, we move it to the top of the list, when it
 * is destroyed, we remove it.
 *
 * NB: the global tab list is a fallback when we cannot use the Switcher list
 * (e.g., for fast Alt+Tab switching without the Switcher.
 */
static void
tablist_meta_window_focus_cb (MetaWindow *mw, gpointer data)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (data)->priv;

  /*
   * Push to the top of the global tablist.
   */
  priv->global_tab_list = g_list_remove (priv->global_tab_list, mw);
  priv->global_tab_list = g_list_prepend (priv->global_tab_list, mw);
}

static void
tablist_meta_window_weak_ref_cb (gpointer data, GObject *mw)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (data)->priv;

  priv->global_tab_list = g_list_remove (priv->global_tab_list, mw);
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

          g_signal_connect (background, "hide-begin",
                            G_CALLBACK (on_config_actor_hide_begin_cb),
                            hide_data);

          g_signal_connect_data (background, "hide-completed",
                                 G_CALLBACK (on_config_actor_hide_completed_cb),
                                 hide_data,
                                 (GClosureNotify)g_free, 0);

          clutter_actor_set_position (background, x, y);

          g_object_set (actor, "no-shadow", TRUE, NULL);

          clutter_actor_hide (actor);

          parent = mutter_plugin_get_overlay_group (plugin);

          clutter_container_add_actor (CLUTTER_CONTAINER (priv->panel),
                                       background);
          /*
           * Raise the tray just above all the other dropdowns, but below
           * the actual panel background (the switcher is the topmost dropdown).
           */
          clutter_actor_raise (background, priv->switcher);
          clutter_actor_show_all (background);
        }
      else
        mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);

    }
  else if (type == META_COMP_WINDOW_NORMAL ||
           type == META_COMP_WINDOW_SPLASHSCREEN)
    {
      ClutterAnimation *animation;
      EffectCompleteData *data = g_new0 (EffectCompleteData, 1);
      ActorPrivate *apriv = get_actor_private (mcw);
      MetaWindow   *mw = mutter_window_get_meta_window (mcw);

      if (mw)
        {
          const char *sn_id = meta_window_get_startup_id (mw);

          if (!moblin_netbook_sn_should_map (plugin, mcw, sn_id))
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
                            G_CALLBACK (tablist_meta_window_focus_cb),
                            plugin);

          g_object_weak_ref (G_OBJECT (mw),
                             tablist_meta_window_weak_ref_cb,
                             plugin);
        }
    }
  else
    mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);

}

/*
 * Destroy effect completion callback; this is a simple effect that requires no
 * further action than notifying the manager that the effect is completed.
 */
static void
on_destroy_effect_complete (ClutterTimeline *timeline, EffectCompleteData *data)
{
  MutterPlugin *plugin = data->plugin;
  MutterWindow *mcw    = MUTTER_WINDOW (data->actor);
  ActorPrivate *apriv  = get_actor_private (mcw);

  apriv->tml_destroy = NULL;

  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_DESTROY);
}


/*
 * Simple TV-out like effect.
 */
static void
destroy (MutterPlugin *plugin, MutterWindow *mcw)
{
  MetaScreen                 *screen;
  MetaCompWindowType          type;
  ClutterActor               *actor = CLUTTER_ACTOR (mcw);
  gint                        workspace;
  MetaWindow                 *meta_win;

  type      = mutter_window_get_window_type (mcw);
  workspace = mutter_window_get_workspace (mcw);
  meta_win  = mutter_window_get_meta_window (mcw);
  screen    = mutter_plugin_get_screen (plugin);

  if (type == META_COMP_WINDOW_NORMAL)
    {
      ClutterAnimation *animation;
      EffectCompleteData *data = g_new0 (EffectCompleteData, 1);

      clutter_actor_move_anchor_point_from_gravity (actor,
                                                    CLUTTER_GRAVITY_CENTER);

      animation = clutter_actor_animate (actor,
                                         CLUTTER_EASE_IN_SINE,
                                         DESTROY_TIMEOUT,
                                         "scale-x", 0.0,
                                         "scale-y", 0.0,
                                         NULL);
      data->plugin = plugin;
      data->actor = actor;
      g_signal_connect (clutter_animation_get_timeline (animation),
                        "completed",
                        G_CALLBACK (on_destroy_effect_complete),
                        data);

    }
  else
    mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_DESTROY);

  /*
   * Do not destroy workspace if the closing window is a splash screen.
   * (Sometimes the splash gets destroyed before the application window
   * maps, e.g., Gimp.)
   */
  if (type != META_COMP_WINDOW_SPLASHSCREEN)
    check_for_empty_workspace (plugin, workspace, meta_win);
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

/*
 * Use this function to disable stage input
 *
 * Used by the completion callback for the panel in/out effects
 */
void
disable_stage (MutterPlugin *plugin, guint32 timestamp)
{
  MoblinNetbookPluginPrivate *priv    = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);
  MetaWindow                 *focus;

  /*
   * Refuse to disable the stage while the UI is showing.
   */
  if (CLUTTER_ACTOR_IS_VISIBLE (priv->panel) ||
      priv->panel_out_in_progress ||
      priv->workspace_chooser)
    {
      g_warning ("Cannot disable stage while the panel/chooser is showing\n");
      return;
    }

  if (timestamp == CurrentTime)
    timestamp = clutter_x11_get_current_event_time ();

  priv->current_input_base_region = priv->panel_trigger_region;

  moblin_netbook_input_region_apply (plugin);

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

  if (focus)
    meta_display_set_input_focus_window (display, focus, FALSE, timestamp);

  priv->blocking_input = FALSE;
}

void
enable_stage (MutterPlugin *plugin, guint32 timestamp)
{
  MoblinNetbookPluginPrivate *priv    = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);

  if (timestamp == CurrentTime)
    timestamp = clutter_x11_get_current_event_time ();

  priv->current_input_base_region = priv->screen_region;

  moblin_netbook_input_region_apply (plugin);

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

  meta_display_focus_the_no_focus_window (display, screen, timestamp);

  priv->blocking_input = TRUE;
}

static gboolean
xevent_filter (MutterPlugin *plugin, XEvent *xev)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  if (priv->in_alt_grab)
    {
      if (xev->type == KeyRelease &&
          XKeycodeToKeysym (xev->xkey.display,
                            xev->xkey.keycode, 0) == XK_Alt_L)
        {
          MetaScreen  *screen  = mutter_plugin_get_screen (plugin);
          MetaDisplay *display = meta_screen_get_display (screen);

          meta_display_end_grab_op (display, xev->xkey.time);
          priv->in_alt_grab = FALSE;

          mnb_switcher_activate_selection (MNB_SWITCHER (priv->switcher), TRUE,
                                           xev->xkey.time);
          return TRUE;
        }

      /*
       * Block processing of pointer events by clutter. We do not want the
       * user to be doing stuff like d&d, etc., while in grab mode.
       */
      if (xev->type == ButtonPress   ||
          xev->type == ButtonRelease ||
          xev->type == MotionNotify)
        return TRUE;
    }

  if (xev->type == KeyPress &&
      XKeycodeToKeysym (xev->xkey.display, xev->xkey.keycode, 0) ==
                                                    MOBLIN_PANEL_SHORTCUT_KEY)
    {
      if (!CLUTTER_ACTOR_IS_VISIBLE (priv->panel))
        show_panel (plugin, TRUE);
      else
        {
          priv->panel_wait_for_pointer = FALSE;
          hide_panel (plugin);
        }

      return TRUE;
    }

  sn_display_process_event (priv->sn_display, xev);

  if (!priv->in_alt_grab &&
      (xev->type == KeyPress || xev->type == KeyRelease))
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
      MoblinNetbookPluginPrivate *ppriv  = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

      /*
       * Force emission of the "completed" signal so that the necessary
       * cleanup is done (we cannot readily supply the data necessary to
       * call our callback directly).
       */

      if (ppriv->tml_switch_workspace0)
        {
          clutter_timeline_stop (ppriv->tml_switch_workspace0);
          g_signal_emit_by_name (ppriv->tml_switch_workspace0, "completed");
        }

      if (ppriv->tml_switch_workspace1)
        {
          clutter_timeline_stop (ppriv->tml_switch_workspace1);
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

  if ((event & MUTTER_PLUGIN_DESTROY) && apriv->tml_destroy)
    {
      clutter_timeline_stop (apriv->tml_destroy);
      g_signal_emit_by_name (apriv->tml_destroy, "completed", NULL);
    }
}

static gboolean
panel_slide_timeout_cb (gpointer data)
{
  MutterPlugin  *plugin = data;
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  if (priv->last_y < PANEL_SLIDE_THRESHOLD)
    {
      show_panel (plugin, FALSE);
    }
  else
    {
      disable_stage (plugin, CurrentTime);
    }

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

      priv->last_y = event_y;

      if (priv->panel_out_in_progress || priv->panel_back_in_progress)
        return FALSE;

      if (CLUTTER_ACTOR_IS_VISIBLE (priv->panel) &&
          ((!CLUTTER_ACTOR_IS_VISIBLE (priv->switcher) &&
            !CLUTTER_ACTOR_IS_VISIBLE (priv->launcher) &&
            !CLUTTER_ACTOR_IS_VISIBLE (priv->mzone_grid) &&
            !CLUTTER_ACTOR_IS_VISIBLE (priv->status))))
        {
          /*
           * FIXME -- we should use the height of the panel background here;
           *          when we refactor the panel code, we should expose that
           *          value as a property on the object.
           */
          guint height = clutter_actor_get_height (priv->panel_shadow);

          if (event_y > (gint)height)
            {
              hide_panel (MUTTER_PLUGIN (plugin));
            }
        }
      else if (event_y < PANEL_SLIDE_THRESHOLD)
        {
          if (!priv->panel_slide_timeout_id &&
              !CLUTTER_ACTOR_IS_VISIBLE (priv->panel) &&
              !priv->workspace_chooser)
            {
              priv->current_input_base_region = priv->panel_trigger_region2;
              moblin_netbook_input_region_apply (MUTTER_PLUGIN (plugin));

              priv->panel_slide_timeout_id =
                g_timeout_add (PANEL_SLIDE_THRESHOLD_TIMEOUT,
                               panel_slide_timeout_cb, plugin);
            }
        }
    }
  else if (event->any.source == stage &&
           (event->type == CLUTTER_ENTER || event->type == CLUTTER_LEAVE))
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

      if (CLUTTER_ACTOR_IS_VISIBLE (priv->switcher))
        clutter_actor_hide (priv->switcher);

      if (priv->launcher)
        clutter_actor_hide (priv->launcher);

      if (priv->mzone_grid)
        clutter_actor_hide (priv->mzone_grid);

      shell_tray_manager_close_all_config_windows (priv->tray_manager);

      if (CLUTTER_ACTOR_IS_VISIBLE (priv->panel))
        {
          guint height = clutter_actor_get_height (priv->panel_shadow);

          if (event_y > (gint)height)
            {
              hide_panel (plugin);
            }
        }
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
  Window                      parent_win;
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
                            0, 0, 1, 1, 0,
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
      ClutterActor *bg_clone;
      ClutterActor *stage = mutter_get_stage_for_screen (screen);

      g_object_set (priv->parallax_tex,
                    "repeat-x", TRUE,
                    "repeat-y", TRUE,
                    NULL);

      bg_clone = clutter_clone_new (priv->parallax_tex);
      clutter_actor_set_size (bg_clone, screen_width, screen_height);
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), bg_clone);
      clutter_actor_lower_bottom (bg_clone);
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

/*
 * The machinery for manipulating shape of the stage window.
 *
 * The problem: we are mixing UI done in Clutter with applications using X.
 * When the Clutter stage window receives input events, the X windows (located
 * beneath it) do not. At time we want events on parts of the screen to be
 * handled by Clutter and parts by X; for that we have to change the input
 * shape of the stage window (for which mutter provides API).
 *
 * The are two basic states we start with.
 *
 * (a) All events go to clutter, and nothing to X; this is generally the case
 *     when the UI is visible.
 *
 * (b) All events except for a 1px high strip along the top of the screen go
 *     to X. This is the case when the UI is hidden (the 1px strip is a trigger
 *     zone for showing the pannel).
 *
 * Basic state (a) is represented by screen_region (in the plugin private),
 * state (b) by panel_trigger_region.
 *
 * There are situations in which neither of the basic states is what we
 * require. Sometimes when in state (a) we need to 'cut out' additional holes in
 * to allow events reaching X (e.g., when showing system tray config windows),
 * sometimes when in state (b) we need events at parts of the screen to be
 * handled by Clutter (e.g., the notifications).
 *
 * We can use the XFixes API to modify the input shape to match our
 * requirements; the one complication in this is that while modifying the
 * existing shape is easy (a simple union or subtraction), such modification
 * cannot be trivially undone. For example, let's start with initial state (b):
 *
 *   1. All events go to X, the input shape is empty.
 *
 *   2. Notification appears requring area A to be added to the input shape.
 *
 *   3. User opens Panel; the new required shape is the union of basic state
 *      (a) and area A require by notification (a u A).
 *
 *   4. User deals with notification, which closes; at this point we need to
 *      reverse the union (a u A); unfortunatley, union operation is not
 *      reversible unless we know what the consituent parts of the union were
 *      (e.g., subtracting A from ours shape would leave us with a hole where
 *      A was).
 *
 * So, we maintain a stack of regions that make up the current input shape. This
 * stack consists of:
 *
 *   1. Base region, current_input_base region. This points either to
 *      screen_region (we are in mode (a)), or the panel_trigger_region (mode
 *      (b)).
 *
 *   2. Stack of custom regions required by UI components; each of these can
 *      either be additive (events here go to Clutter) or inverse (events go
 *      to X).
 *
 *   3. The regions are combined in the order in which they were pushed on the
 *      stack, starting from the base. The resulting region is used as the input
 *      shape for stage window.
 *
 *   4. Each region remains on the stack until it is explicitely removed, using
 *      to region ID obtained during the stack push.
 *
 * The individual functions are commented on below.
 */
struct MnbInputRegion
{
  XserverRegion region;
  gboolean      inverse;
};

/*
 * moblin_netbook_input_region_push()
 *
 * Puhses region of the given dimensions onto the input region stack; this is
 * immediately reflected in the actual input shape.
 *
 * x, y, width, height: region position and size (screen-relative)
 *
 * inverse: indicates whether the region should be added to the input shape
 *          (FALSE; input events in this area will go to Clutter) or subtracted
 *          from it (TRUE; input events will go to X)
 *
 * returns: id that identifies this region; used in subsequent call to
 *          moblin_netbook_input_region_remove().
 */
MnbInputRegion
moblin_netbook_input_region_push (MutterPlugin *plugin,
                                  gint          x,
                                  gint          y,
                                  guint         width,
                                  guint         height,
                                  gboolean      inverse)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  MnbInputRegion mir = g_slice_alloc (sizeof (struct MnbInputRegion));
  XRectangle     rect;
  Display       *xdpy = mutter_plugin_get_xdisplay (plugin);

  rect.x       = x;
  rect.y       = y;
  rect.width   = width;
  rect.height  = height;

  mir->inverse = inverse;
  mir->region  = XFixesCreateRegion (xdpy, &rect, 1);

  priv->input_region_stack = g_list_append (priv->input_region_stack, mir);

  if (inverse)
    XFixesSubtractRegion (xdpy,
                          priv->current_input_region,
                          priv->current_input_region, mir->region);
  else
    XFixesUnionRegion (xdpy,
                       priv->current_input_region,
                       priv->current_input_region, mir->region);

  mutter_plugin_set_stage_input_region (plugin, priv->current_input_region);

  return mir;
}

/*
 * moblin_netbook_input_region()
 *
 * Removes region previously pushed onto the stack with
 * moblin_netbook_input_region_push(). This change is immediately applied to the
 * actual input shape.
 *
 * mir: the region ID returned by moblin_netbook_input_region_push().
 */
void
moblin_netbook_input_region_remove (MutterPlugin *plugin, MnbInputRegion mir)
{
  moblin_netbook_input_region_remove_without_update (plugin, mir);
  moblin_netbook_input_region_apply (plugin);
}

/*
 * moblin_netbook_input_region_remove_without_update()
 *
 * Removes region previously pushed onto the stack with
 * moblin_netbook_input_region_push(). This changes does not immediately
 * filter into the actual input shape; this is useful if you need to
 * replace an existing region, as it saves round trip to the server.
 *
 * mir: the region ID returned by moblin_netbook_input_region_push().
 */
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

/*
 * Applies the current input shape base and stack to the stage input shape.
 * This function is for internal use only and should not be used outside of the
 * actual implementation of the input shape stack.
 */
static void
moblin_netbook_input_region_apply (MutterPlugin *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  Display *xdpy = mutter_plugin_get_xdisplay (plugin);
  GList *l = priv->input_region_stack;
  XserverRegion result = priv->current_input_region;

  XFixesCopyRegion (xdpy, result, priv->current_input_base_region);

  while (l)
    {
      MnbInputRegion mir = l->data;

      if (mir->inverse)
        XFixesSubtractRegion (xdpy, result, result, mir->region);
      else
        XFixesUnionRegion (xdpy, result, result, mir->region);

      l = l->next;
    }

  mutter_plugin_set_stage_input_region (plugin, result);
}


