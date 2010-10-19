/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008, 2010 Intel Corp.
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

#include "meego-netbook.h"
#include "alttab/mnb-alttab-overlay.h"
#include "mnb-toolbar.h"
#include "effects/mnb-switch-zones-effect.h"
#include "notifications/mnb-notification-gtk.h"
#include "notifications/ntf-overlay.h"
#include "presence/mnb-presence.h"
#include "mnb-panel-frame.h"
#include "meego-netbook-constraints.h"
#include "meego-netbook-mutter-hints.h"
#include "notifications/ntf-overlay.h"

#include <compositor-mutter.h>
#include <display.h>
#include <prefs.h>
#include <keybindings.h>
#include <errors.h>
#include <X11/extensions/scrnsaver.h>

/*
 * Including mutter's errors.h defines the i18n macros, so undefine them before
 * including the glib i18n header.
 */
#undef _
#undef N_

#include <glib/gi18n.h>
#include <gdk/gdkx.h>

#include <meego-panel/mpl-panel-common.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gmodule.h>
#include <string.h>
#include <signal.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrandr.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#define MINIMIZE_TIMEOUT            250
#define MAXIMIZE_TIMEOUT            250
#define MAP_TIMEOUT                 350
#define SWITCH_TIMEOUT              400
#define WS_SWITCHER_SLIDE_TIMEOUT   250
#define MYZONE_TIMEOUT              200
#define ACTOR_DATA_KEY "MCCP-meego-netbook-actor-data"
#define BG_KEY_DIR "/desktop/gnome/background"
#define KEY_BG_FILENAME BG_KEY_DIR "/picture_filename"
#define KEY_BG_OPTIONS BG_KEY_DIR "/picture_options"
#define THEME_KEY_DIR "/apps/metacity/general"
#define KEY_THEME THEME_KEY_DIR "/theme"
#define KEY_BUTTONS THEME_KEY_DIR "/button_layout"
#define KEY_ALLWAYS_SMALL_SCREEN "/desktop/meego/always_small_screen_mode"

static guint32 compositor_options = 0;

static const GDebugKey options_keys[] =
{
  { "compositor-always-on",       MNB_OPTION_COMPOSITOR_ALWAYS_ON },
  { "disable-ws-clamp",           MNB_OPTION_DISABLE_WS_CLAMP },
  { "disable-panel-restart",      MNB_OPTION_DISABLE_PANEL_RESTART },
  { "composite-fullscreen-apps",  MNB_OPTION_COMPOSITE_FULLSCREEN_APPS },
};

static MutterPlugin *plugin_singleton = NULL;

/* callback data for when animations complete */
typedef struct
{
  ClutterActor *actor;
  MutterPlugin *plugin;
} EffectCompleteData;

static void desktop_background_init (MutterPlugin *plugin);
static void setup_focus_window (MutterPlugin *plugin);
static void setup_screen_saver (MutterPlugin *plugin);

static void fullscreen_app_added (MutterPlugin *, MetaWindow *);
static void fullscreen_app_removed (MutterPlugin *, MetaWindow *);
static void meta_window_fullscreen_notify_cb (GObject    *object,
                                              GParamSpec *spec,
                                              gpointer    data);
static void meego_netbook_toggle_compositor (MutterPlugin *, gboolean on);
static void window_destroyed_cb (MutterWindow *mcw, MutterPlugin *plugin);
static void meego_netbook_handle_screen_size (MutterPlugin *plugin,
                                               gint         *screen_width,
                                               gint         *screen_height);

static void last_focus_weak_notify_cb (gpointer data, GObject *meta_win);

static GQuark actor_data_quark = 0;

static void     minimize   (MutterPlugin *plugin,
                            MutterWindow *actor);
static void     map        (MutterPlugin *plugin,
                            MutterWindow *actor);
static void     destroy    (MutterPlugin *plugin,
                            MutterWindow *actor);
static void     switch_workspace (MutterPlugin         *plugin,
                                  gint                  from,
                                  gint                  to,
                                  MetaMotionDirection   direction);
static void     maximize   (MutterPlugin *plugin,
                            MutterWindow *actor,
                            gint x, gint y, gint width, gint height);
static void     unmaximize (MutterPlugin *plugin,
                            MutterWindow *actor,
                            gint x, gint y, gint width, gint height);

static void     kill_window_effects (MutterPlugin *plugin,
                                     MutterWindow *actor);
static void     kill_switch_workspace (MutterPlugin *plugin);

static const MutterPluginInfo * plugin_info (MutterPlugin *plugin);

static gboolean xevent_filter (MutterPlugin *plugin, XEvent *xev);

MUTTER_PLUGIN_DECLARE (MeegoNetbookPlugin, meego_netbook_plugin);

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
meego_netbook_plugin_dispose (GObject *object)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (object)->priv;

  if (priv->toolbar)
    {
      clutter_actor_destroy (priv->toolbar);
      priv->toolbar = NULL;
    }

  G_OBJECT_CLASS (meego_netbook_plugin_parent_class)->dispose (object);
}

static void
meego_netbook_plugin_finalize (GObject *object)
{
  mnb_input_manager_destroy ();

  G_OBJECT_CLASS (meego_netbook_plugin_parent_class)->finalize (object);
}

static void
meego_netbook_plugin_set_property (GObject      *object,
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
meego_netbook_plugin_get_property (GObject    *object,
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
 * If the work area size changes while a panel is present (e.g., a VKB pops up),
 * we need to resize the panel. We also need to reposition any elements the
 * position of which is fixed and depends on the screen size.
 */
static void
meego_netbook_workarea_changed_cb (MetaScreen *screen, MutterPlugin *plugin)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  gint                        screen_width, screen_height;

  meego_netbook_handle_screen_size (plugin, &screen_width, &screen_height);

  if (priv->desktop_tex)
    clutter_actor_set_size (priv->desktop_tex, screen_width, screen_height);
}

static void
meego_netbook_overlay_key_cb (MetaDisplay *display, MutterPlugin *plugin)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;

  if (meego_netbook_urgent_notification_present (plugin))
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
      mnb_toolbar_show (MNB_TOOLBAR (priv->toolbar), MNB_SHOW_HIDE_BY_KEY);
    }
  else
    {
      mnb_toolbar_hide (MNB_TOOLBAR (priv->toolbar), MNB_SHOW_HIDE_BY_KEY);
    }
}

static gboolean
meego_netbook_fullscreen_apps_present_on_workspace (MutterPlugin *plugin,
                                                     gint          index)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
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
meego_netbook_workspace_switched_cb (MetaScreen          *screen,
                                      gint                 from,
                                      gint                 to,
                                      MetaMotionDirection  dir,
                                      MutterPlugin        *plugin)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  gboolean on;

  if (compositor_options & MNB_OPTION_COMPOSITE_FULLSCREEN_APPS)
    return;

  if (priv->screen_saver_dpms || priv->screen_saver_mcw)
    return;

  on = !meego_netbook_fullscreen_apps_present_on_workspace (plugin, to);

  meego_netbook_toggle_compositor (plugin, on);
}

static void
meego_netbook_compute_screen_size (Display  *xdpy,
                                    gint      screen_no,
                                    gint     *width_mm,
                                    gint     *height_mm,
                                    gboolean *external)
{
  Window                  xroot = RootWindow (xdpy, screen_no);
  XRRScreenSize          *sizes;
  XRRScreenConfiguration *cfg = NULL;
  SizeID                  current;
  Rotation                rotation;
  gint                    n_sizes;

  cfg = XRRGetScreenInfo (xdpy, xroot);

  if (!cfg)
    goto fallback;

  current = XRRConfigCurrentConfiguration (cfg, &rotation);

  sizes = XRRConfigSizes (cfg, &n_sizes);

  if (current < 0 || !sizes)
    goto fallback;

  *width_mm  = sizes[current].mwidth;
  *height_mm = sizes[current].mheight;

  XRRFreeScreenConfigInfo (cfg);

  *external = FALSE;

  {
    XRRScreenResources *res = XRRGetScreenResourcesCurrent (xdpy, xroot);
    gint i;

    for (i = 0; i < res->noutput; ++i)
      {
        XRROutputInfo *info = XRRGetOutputInfo (xdpy, res, res->outputs[i]);

        /*
         * Must be both connected and have crtc associated, BMC#3795
         */
        if (info->connection == RR_Connected && info->crtc)
          {
            if (strncmp (info->name, "LVDS", strlen ("LVDS")))
              *external = TRUE;

            XRRFreeOutputInfo (info);
          }
      }

    XRRFreeScreenResources (res);
  }

  return;

 fallback:
  if (cfg)
    XRRFreeScreenConfigInfo (cfg);

  g_warning ("Could not retrieve screen info via xrandr");

  /*
   * Fall back on the server info; this is so inaccurate that it is
   * useless.
   */
  *width_mm  = XDisplayWidthMM  (xdpy, screen_no);
  *height_mm = XDisplayHeightMM (xdpy, screen_no);
}

static void
meego_netbook_panel_window_show_cb (MutterWindow *mcw, MutterPlugin *plugin)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  MnbToolbar                 *toolbar = MNB_TOOLBAR (priv->toolbar);
  MnbPanel                   *panel;
  Window                      xwin = mutter_window_get_x_window (mcw);

  if ((panel = mnb_toolbar_find_panel_for_xid (toolbar, xwin)))
    mnb_panel_oop_show_mutter_window (MNB_PANEL_OOP (panel), mcw);

  g_signal_handlers_disconnect_by_func (mcw,
                                        meego_netbook_panel_window_show_cb,
                                        plugin);
}

static void
meego_netbook_display_window_created_cb (MetaDisplay  *display,
                                          MetaWindow   *win,
                                          MutterPlugin *plugin)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  MnbToolbar                 *toolbar = MNB_TOOLBAR (priv->toolbar);
  MutterWindow               *mcw;
  MetaCompWindowType          type;
  MnbPanel                   *panel;

  mcw =  (MutterWindow*) meta_window_get_compositor_private (win);

  g_return_if_fail (mcw);

  type = mutter_window_get_window_type (mcw);

  if (type == META_COMP_WINDOW_DOCK)
    {
      MnbPanel    *panel;
      Window       xwin = mutter_window_get_x_window (mcw);
      const gchar *wm_class;

      if ((panel = mnb_toolbar_find_panel_for_xid (toolbar, xwin)))
        {
          /*
           * Order matters; mnb_panel_show_mutter_window() hides the mcw,
           * and since the compositor call clutter_actor_show() when a map
           * effect completes, we have to first let the compositor to do its
           * thing, and only then impose our will on it.
           */
          g_object_set (mcw, "shadow-type", MUTTER_SHADOW_ALWAYS, NULL);

          g_signal_connect (mcw, "show",
                            G_CALLBACK (meego_netbook_panel_window_show_cb),
                            plugin);
        }
      else if ((wm_class = meta_window_get_wm_class (win)) &&
               !strcmp (wm_class, "Matchbox-panel"))
        {
          g_object_set (mcw, "shadow-type", MUTTER_SHADOW_ALWAYS, NULL);
        }
    }

  if (mutter_window_is_override_redirect (mcw))
    {
      const gchar *wm_class = meta_window_get_wm_class (win);

      if (wm_class && !strcmp (wm_class, "Gnome-screensaver"))
        {
          priv->screen_saver_mcw = mcw;
          meego_netbook_toggle_compositor (plugin, FALSE);

          g_signal_connect (mcw, "window-destroyed",
                            G_CALLBACK (window_destroyed_cb),
                            plugin);
        }
    }

  if ((panel = mnb_toolbar_get_active_panel (toolbar)) &&
      MNB_IS_PANEL_OOP (panel) &&
      mnb_panel_oop_owns_window ((MnbPanelOop*)panel, mcw))
    {
      mnb_input_manager_push_window (mcw, MNB_INPUT_LAYER_PANEL);
    }
}

static void
meego_netbook_display_focus_window_notify_cb (MetaDisplay  *display,
                                               GParamSpec   *spec,
                                               MutterPlugin *plugin)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  MetaWindow                 *mw   = meta_display_get_focus_window (display);

  if (mw && priv->last_focused != mw)
    {
      MutterWindow       *mcw;
      MetaCompWindowType  type;
      Window              xwin;

      mcw  = (MutterWindow*) meta_window_get_compositor_private (mw);
      type = mutter_window_get_window_type (mcw);
      xwin = meta_window_get_xwindow (mw);

      /*
       * Ignore panels and the toolbar focus window.
       */
      if (type != META_COMP_WINDOW_DOCK &&
          xwin != priv->focus_xwin      &&
          !meta_display_xwindow_is_a_no_focus_window (display, xwin))
        {
          if (priv->last_focused)
            {
              g_object_weak_unref (G_OBJECT (priv->last_focused),
                                   last_focus_weak_notify_cb, plugin);

              priv->last_focused = NULL;
            }

          priv->last_focused = mw;

          if (mw)
            g_object_weak_ref (G_OBJECT (mw),
                               last_focus_weak_notify_cb, plugin);
        }
    }
}

static void
meego_netbook_handle_screen_size (MutterPlugin *plugin,
                                   gint         *screen_width,
                                   gint         *screen_height)
{
  MeegoNetbookPluginPrivate *priv   = MEEGO_NETBOOK_PLUGIN (plugin)->priv;

  MnbToolbar   *toolbar   = (MnbToolbar*)priv->toolbar;
  MetaScreen   *screen    = mutter_plugin_get_screen (MUTTER_PLUGIN (plugin));
  MetaDisplay  *display   = meta_screen_get_display (screen);
  Display      *xdpy      = meta_display_get_xdisplay (display);
  ClutterActor *stage     = mutter_get_stage_for_screen (screen);
  guint         screen_no = meta_screen_get_screen_number (screen);
  gint          screen_width_mm  = XDisplayWidthMM (xdpy, screen_no);
  gint          screen_height_mm = XDisplayHeightMM (xdpy, screen_no);
  gchar        *meego_session;
  Window        leader_xwin;
  gboolean      netbook_mode;
  gboolean      external = FALSE;
  gboolean      force_small_screen = FALSE;

  static Atom   atom__MEEGO = None;
  static gint   old_screen_width = 0, old_screen_height = 0;

  /*
   * Because we are hooked into the workareas-changed signal, we get typically
   * called twice for each change; the first time when the monitor resizes,
   * and the second time if/when the Toolbar adusts it's struts. So do a check
   * here that the 'new' state is different from the old one.
   */
  mutter_plugin_query_screen_size (plugin, screen_width, screen_height);

  if (old_screen_width == *screen_width && old_screen_height == *screen_height)
    return;

  old_screen_width  = *screen_width;
  old_screen_height = *screen_height;

  force_small_screen = gconf_client_get_bool (priv->gconf_client,
                                              KEY_ALLWAYS_SMALL_SCREEN,
                                              NULL);

  if (!force_small_screen)
    {
      meego_netbook_compute_screen_size (xdpy,
                                          screen_no,
                                          &screen_width_mm,
                                          &screen_height_mm,
                                          &external);

      g_debug ("Screen size %dmm x %dmm, external %d",
               screen_width_mm, screen_height_mm, external);
    }

  if (force_small_screen || (!external && screen_width_mm < 280))
    netbook_mode = priv->netbook_mode = TRUE;
  else
    netbook_mode = priv->netbook_mode = FALSE;

  if (!atom__MEEGO)
    atom__MEEGO = XInternAtom (xdpy, "_MEEGO", False);

  leader_xwin = meta_display_get_leader_window (display);

  meego_session =
    g_strdup_printf ("session-type=%s",
                     netbook_mode ? "small-screen" : "bigger-screen");

  g_debug ("Setting _MEEGO=%s", meego_session);

  meta_error_trap_push (display);
  XChangeProperty (xdpy,
                   leader_xwin,
                   atom__MEEGO,
                   XA_STRING,
                   8, PropModeReplace,
                   (unsigned char*)meego_session, strlen (meego_session));
  meta_error_trap_pop (display, FALSE);

  g_free (meego_session);

  gconf_client_set_string (priv->gconf_client, KEY_THEME,
                           netbook_mode ? "Netbook" : "Nettop",
                           NULL);

  gconf_client_set_string (priv->gconf_client, KEY_BUTTONS,
                           netbook_mode ? ":close" : ":maximize,close",
                           NULL);

  /*
   * It seems that sometimes the screen resize gets delayed until something
   * else 'traumatic' happens, such workspace switch; this is an attempt to
   * force the size to resize.
   */
  clutter_actor_queue_redraw (stage);

  if (!netbook_mode &&
      CLUTTER_ACTOR_IS_VISIBLE (stage) &&
      !CLUTTER_ACTOR_IS_VISIBLE (toolbar))
    {
      mnb_toolbar_show (toolbar, MNB_SHOW_HIDE_POLICY);
    }
  else if (netbook_mode &&
           CLUTTER_ACTOR_IS_VISIBLE (stage) &&
           CLUTTER_ACTOR_IS_VISIBLE (toolbar) &&
           !mnb_toolbar_get_active_panel (toolbar))
    {
      gint   x, y, root_x, root_y;
      Window root, child;
      guint  mask;
      Window xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

      meta_error_trap_push (display);

      if (XQueryPointer (xdpy, xwin, &root, &child,
                         &root_x, &root_y, &x, &y, &mask))
        {
          if (root_y > TOOLBAR_HEIGHT)
            mnb_toolbar_hide (toolbar, MNB_SHOW_HIDE_POLICY);
        }

      meta_error_trap_pop (display, FALSE);
    }
}

static void
meego_netbook_plugin_start (MutterPlugin *plugin)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;

  ClutterActor *overlay;
  ClutterActor *toolbar;
  ClutterActor *switcher_overlay;
  ClutterActor *message_overlay;
  gint          screen_width, screen_height;
  GError       *err = NULL;

  MetaScreen   *screen    = mutter_plugin_get_screen (plugin);
  MetaDisplay  *display   = meta_screen_get_display (screen);
  ClutterActor *stage     = mutter_get_stage_for_screen (screen);
  GConfClient  *gconf_client;

  plugin_singleton = plugin;

  gconf_client = priv->gconf_client = gconf_client_get_default ();

  meego_netbook_handle_screen_size (plugin, &screen_width, &screen_height);

  /* tweak with env var as then possible to develop in desktop env. */
  if (!(compositor_options & MNB_OPTION_DISABLE_WS_CLAMP))
    meta_prefs_set_num_workspaces (1);

  mx_texture_cache_load_cache (mx_texture_cache_get_default (),
                                 THEMEDIR "/mx.cache");
  mx_style_load_from_file (mx_style_get_default (),
                             THEMEDIR "/mutter-meego.css",
                             &err);
  if (err)
    {
      g_warning ("%s", err->message);
      g_error_free (err);
    }

  g_signal_connect (screen,
                    "workareas-changed",
                    G_CALLBACK (meego_netbook_workarea_changed_cb),
                    plugin);

  g_signal_connect (screen,
                    "workspace-switched",
                    G_CALLBACK (meego_netbook_workspace_switched_cb),
                    plugin);

  g_signal_connect (display,
                    "overlay-key",
                    G_CALLBACK (meego_netbook_overlay_key_cb),
                    plugin);

  g_signal_connect (display,
                    "window-created",
                    G_CALLBACK (meego_netbook_display_window_created_cb),
                    plugin);

  g_signal_connect (display,
                    "notify::focus-window",
                    G_CALLBACK (meego_netbook_display_focus_window_notify_cb),
                    plugin);

  overlay = mutter_plugin_get_overlay_group (plugin);

  mnb_input_manager_create (plugin);

  setup_focus_window (plugin);
  setup_screen_saver (plugin);

  /*
   * This also creates the launcher.
   */
  toolbar = priv->toolbar = CLUTTER_ACTOR (mnb_toolbar_new (plugin));

  switcher_overlay = priv->switcher_overlay =
    CLUTTER_ACTOR (mnb_alttab_overlay_new ());

  clutter_set_motion_events_enabled (TRUE);

  desktop_background_init (plugin);

  /*
   * FIXME -- make both of these screen-sized containers that automatically
   * resize and position their children.
   */
  message_overlay = ntf_overlay_new ();

  /*
   * Order matters:
   *
   *  - toolbar hint is below the toolbar (i.e., not visible if panel showing.
   */
  clutter_container_add (CLUTTER_CONTAINER (overlay),
                         toolbar,
                         switcher_overlay,
                         NULL);

  clutter_container_add (CLUTTER_CONTAINER (stage),
                         message_overlay,
                         NULL);

  clutter_actor_hide (switcher_overlay);

  /*
   * Session presence.  In the future we should just use a lean gnome-session,
   * but for now mutter can be the presence manager.
   */
  presence_init ((MeegoNetbookPlugin*)plugin);

  /* Keys */

  meta_prefs_override_no_tab_popup (TRUE);
}

static MutterShadow *
meego_netbook_get_shadow (MutterPlugin *plugin, MutterWindow *mcw)
{
  MetaCompWindowType type;

  type = mutter_window_get_window_type (mcw);

  if (type == META_COMP_WINDOW_DOCK)
    {
      MutterShadow *shadow = mutter_shadow_new ();
      MxPadding     padding;

      shadow->actor = (ClutterActor*) mnb_panel_frame_new ();

      mx_stylable_style_changed (MX_STYLABLE (shadow->actor),
                                 MX_STYLE_CHANGED_FORCE);

      mx_widget_get_padding (MX_WIDGET (shadow->actor), &padding);

      shadow->attach_left   = -padding.left;
      shadow->attach_top    = -padding.top;
      shadow->attach_right  =  padding.right;
      shadow->attach_bottom =  padding.bottom;

      return shadow;
    }

  return NULL;
}

static void
meego_netbook_plugin_class_init (MeegoNetbookPluginClass *klass)
{
  GObjectClass      *gobject_class = G_OBJECT_CLASS (klass);
  MutterPluginClass *plugin_class  = MUTTER_PLUGIN_CLASS (klass);

  gobject_class->finalize        = meego_netbook_plugin_finalize;
  gobject_class->dispose         = meego_netbook_plugin_dispose;
  gobject_class->set_property    = meego_netbook_plugin_set_property;
  gobject_class->get_property    = meego_netbook_plugin_get_property;

  plugin_class->map              = map;
  plugin_class->minimize         = minimize;
  plugin_class->maximize         = maximize;
  plugin_class->unmaximize       = unmaximize;
  plugin_class->destroy          = destroy;
  plugin_class->switch_workspace = switch_workspace;
  plugin_class->kill_window_effects   = kill_window_effects;
  plugin_class->kill_switch_workspace = kill_switch_workspace;
  plugin_class->plugin_info      = plugin_info;
  plugin_class->xevent_filter    = xevent_filter;
  plugin_class->get_shadow       = meego_netbook_get_shadow;
  plugin_class->constrain_window = meego_netbook_constrain_window;
  plugin_class->start            = meego_netbook_plugin_start;

  g_type_class_add_private (gobject_class, sizeof (MeegoNetbookPluginPrivate));
}

static void
meego_netbook_plugin_init (MeegoNetbookPlugin *self)
{
  MeegoNetbookPluginPrivate *priv;
  const gchar                *options;

  self->priv = priv = MEEGO_NETBOOK_PLUGIN_GET_PRIVATE (self);

  priv->info.name        = _("Meego Netbook Effects");
  priv->info.version     = "0.1";
  priv->info.author      = "Intel Corp.";
  priv->info.license     = "GPL";
  priv->info.description = _("Effects for Meego Netbooks");

  bindtextdomain (GETTEXT_PACKAGE, PLUGIN_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  if ((options = g_getenv("MUTTER_COMPOSITOR_OPTIONS")))
    {
      g_message ("**** COMPOSITOR OPTIONS: %s ****", options);

      compositor_options = g_parse_debug_string (options,
                                                 options_keys,
                                                 G_N_ELEMENTS (options_keys));
    }

  priv->scaled_background = TRUE;
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
  mutter_plugin_minimize_completed (plugin, mcw);
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
    mutter_plugin_minimize_completed (plugin, mcw);
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
  mutter_plugin_maximize_completed (plugin, mcw);

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

  mutter_plugin_maximize_completed (plugin, mcw);
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
  mutter_plugin_unmaximize_completed (plugin, mcw);
}

static void
meego_netbook_move_window_to_workspace (MutterWindow *mcw,
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
meego_netbook_move_window_to_its_workspace (MutterPlugin *plugin,
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

  meego_netbook_move_window_to_workspace (mcw, index, timestamp);
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
  mutter_plugin_map_completed (plugin, mcw);
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
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen = mutter_plugin_get_screen (plugin);
  gboolean                    no_apps = TRUE;
  GList                      *l;

  /*
   * Do not drop Myzone if the Toolbar is visible.
   */
  if (CLUTTER_ACTOR_IS_MAPPED (priv->toolbar))
    return FALSE;

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
    mnb_toolbar_activate_panel (MNB_TOOLBAR (priv->toolbar),
                                "meego-panel-myzone",
                                MNB_SHOW_HIDE_POLICY);

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
  Window      xwin = None;

  /*
   * Mutter now treats all OR windows as sticky, and the -1 will trigger
   * false workspace switch.
   */
  if (workspace < 0)
    return;

  if (ignore)
    xwin = meta_window_get_xwindow (ignore);

  l = mutter_get_windows (screen);

  while (l)
    {
      MutterWindow *m  = l->data;
      MetaWindow   *mw = mutter_window_get_meta_window (m);
      Window        xt = meta_window_get_transient_for_as_xid (mw);

      /*
       * We need to check this window is not the window we are too ignore.
       *
       * If the ignore window was not destroyed (i.e., it was moved to a
       * different workspace, we have to ignore its transients in the test,
       * (because when a window is moved to a different workspace, the WM
       * automatically moves its transients too).
       *
       * If the ignore window was destroyed, we ignore any windows that are
       * claiming to be directly transient to this window (this is to work
       * around broken apps like eog that will destroy their top level window
       * _before_ destroying their transients).
       *
       * We skip over windows that are on all workspaces (they are irrelevant
       * for our purposes, and also because their workspace is the active
       * workspace, we can have a bit of a race here if workspace is being
       * removed when the screen active workspace is not yet updated at this
       * point and we we try to translate it to an index, it blows up on us).
       */
      if (mw != ignore &&
          !meta_window_is_on_all_workspaces (mw) &&
          ((win_destroyed && xwin != xt) ||
           (!win_destroyed && !meta_window_is_ancestor_of_transient (ignore,
                                                                     mw))))
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

      /*
       * Check if we show the myzone; we do this after a short timeout, so that
       * we do not drop the myzone if the application does something like first
       * pop a root password dialog, and only after the dialog closes, creates
       * its main window (see MB#4766).
       */
      g_timeout_add (MYZONE_TIMEOUT, (GSourceFunc)maybe_show_myzone, plugin);
    }
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
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
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

  g_signal_handlers_disconnect_by_func (mcw,
                                        window_destroyed_cb,
                                        plugin);

  if (type == META_COMP_WINDOW_NORMAL ||
      type == META_COMP_WINDOW_DIALOG ||
      type == META_COMP_WINDOW_MODAL_DIALOG)
    {
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
    }

  if (priv->screen_saver_mcw == mcw)
    {
      priv->screen_saver_mcw = NULL;

      if (!priv->screen_saver_dpms)
        {
          gboolean on = TRUE;

          if (!(compositor_options & MNB_OPTION_COMPOSITE_FULLSCREEN_APPS))
            on = !meego_netbook_fullscreen_apps_present (plugin);

          if (on)
            {
              g_debug ("Enabling compositor (gnome-screensaver)");
              meego_netbook_toggle_compositor (plugin, on);
            }
        }
    }

  /*
   * * Do not destroy workspace if the closing window is a splash screen.
   *   (Sometimes the splash gets destroyed before the application window
   *   maps, e.g., Gimp.)
   *
   * * Ignore panel windows,
   *
   * * Do not destroy the workspace, if the closing window belongs
   *   to a Panel,
   *
   * * For everything else run the empty workspace check.
   *
   * NB: This must come before we notify Mutter that the effect completed,
   *     otherwise the destruction of this window will be completed and the
   *     workspace switch effect will crash.
   */

  if (type != META_COMP_WINDOW_SPLASHSCREEN &&
      type != META_COMP_WINDOW_DOCK &&
      !mnb_toolbar_owns_window ((MnbToolbar*)priv->toolbar, mcw))
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

#if 0
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
#endif

  check_for_empty_workspace (plugin, old_workspace, mw, FALSE);
}

static void
fullscreen_app_added (MutterPlugin *plugin, MetaWindow *mw)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  gboolean                    compositor_on;

  priv->fullscreen_wins = g_list_prepend (priv->fullscreen_wins, mw);

  if (compositor_options & MNB_OPTION_COMPOSITE_FULLSCREEN_APPS)
    return;

  if (priv->screen_saver_dpms || priv->screen_saver_mcw)
    return;

  compositor_on = !meego_netbook_fullscreen_apps_present (plugin);
  meego_netbook_toggle_compositor (plugin, compositor_on);
}

static void
fullscreen_app_removed (MutterPlugin *plugin, MetaWindow *mw)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  gboolean                    compositor_on;

  priv->fullscreen_wins = g_list_remove (priv->fullscreen_wins, mw);

  if (compositor_options & MNB_OPTION_COMPOSITE_FULLSCREEN_APPS)
    return;

  if (priv->screen_saver_dpms || priv->screen_saver_mcw)
    return;

  compositor_on = !meego_netbook_fullscreen_apps_present (plugin);
  meego_netbook_toggle_compositor (plugin, compositor_on);
}

gboolean
meego_netbook_fullscreen_apps_present (MutterPlugin *plugin)
{
  MetaScreen *screen = mutter_plugin_get_screen (plugin);
  gint        active;

  active = meta_screen_get_active_workspace_index (screen);

  return meego_netbook_fullscreen_apps_present_on_workspace (plugin, active);
}

static void
meego_netbook_detach_mutter_windows (MetaScreen *screen)
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
meego_netbook_toggle_compositor (MutterPlugin *plugin, gboolean on)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen;
  Display                    *xdpy;
  Window                      xroot;
  Window                      overlay;

  if (compositor_options & MNB_OPTION_COMPOSITOR_ALWAYS_ON)
    return;

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
       * there is brief delay before the Meego background appears, and the
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
      meego_netbook_detach_mutter_windows (screen);
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
 * Returns TRUE if the panel should close when this window appears.
 */
static gboolean
meta_window_is_blessed_window (MetaWindow *mw)
{
  const gchar *wm_class;

  /*
   * Only top-level windows can be blessed.
   */
  if (meta_window_get_transient_for (mw))
    return FALSE;

  /*
   * System modal windows are all blessed as a birth-right.
   */
  if (meta_window_is_modal (mw))
    return TRUE;

  /*
   * Of the commonners, only the following can mingle with the aristocracy
   */
  if ((wm_class = meta_window_get_wm_class (mw)))
    {
      if (!strcmp (wm_class, "Gnome-screenshot") ||
          !strcmp (wm_class, "Nautilus"))
        {
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
meego_netbook_window_is_modal_for_panel (MnbPanelOop *panel,
                                          MetaWindow  *window)
{
  MutterWindow *mcw;
  MetaWindow   *mw;
  MetaWindow   *trans_for;

  /*
   * meta_window_get_transient_for() is quite involved, so only call it if
   * we need to.
   */
  if (!meta_window_is_modal (window) ||
      !(trans_for = meta_window_get_transient_for (window)))
    {
      return FALSE;
    }

  mcw = mnb_panel_oop_get_mutter_window (panel);
  mw  = mutter_window_get_meta_window (mcw);

  if (trans_for == mw)
    return TRUE;

  return FALSE;
}

static void
meego_netbook_panel_modal_destroyed_cb (MutterWindow *mcw,
                                         MnbPanelOop  *panel)
{
  mnb_panel_oop_set_auto_modal (panel, FALSE);
}

static gboolean
meego_netbook_never_move_window (MetaWindow *mw)
{
  const gchar *class    = meta_window_get_wm_class (mw);
  const char  *instance = meta_window_get_wm_class_instance (mw);

  if (class && instance)
    {
      if (!strcmp (class, "Nautilus") && !strcmp (instance, "file_progress"))
        return TRUE;
    }

  return FALSE;
}

/*
 * Simple map handler: it applies a scale effect which must be reversed on
 * completion).
 */
static void
map (MutterPlugin *plugin, MutterWindow *mcw)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  MnbToolbar                 *toolbar = MNB_TOOLBAR (priv->toolbar);
  ClutterActor               *actor = CLUTTER_ACTOR (mcw);
  MetaCompWindowType          type;
  MnbPanelOop                *active_panel;
  Window                      xwin;
  MetaWindow                 *mw;
  gboolean                    fullscreen = FALSE;
  gboolean                    move_window = FALSE;

  active_panel = (MnbPanelOop*) mnb_toolbar_get_active_panel (toolbar);
  type         = mutter_window_get_window_type (mcw);
  xwin         = mutter_window_get_x_window (mcw);
  mw           = mutter_window_get_meta_window (mcw);

  if (active_panel &&
      meego_netbook_window_is_modal_for_panel (active_panel, mw))
    {
      mnb_panel_oop_set_auto_modal ((MnbPanelOop*)active_panel, TRUE);
      g_signal_connect (mcw, "window-destroyed",
                        G_CALLBACK (meego_netbook_panel_modal_destroyed_cb),
                        active_panel);
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

  /*
   * The OR test must come before the type test since GTK_WINDOW_POPUP type
   * windows are both override redirect, but also have a _NET_WM_WINDOW_TYPE set
   * to NORMAL
   */
  if (mutter_window_is_override_redirect (mcw))
    {
      const gchar *wm_class = meta_window_get_wm_class (mw);

      mutter_plugin_map_completed (plugin, mcw);

      /*
       * If this is a SCIM window, ensure we have an input region for it on the
       * top of the stack (so that events get through even if a panel is
       * present).
       */
      if (wm_class && !strcmp (wm_class, "Scim-panel-gtk"))
        mnb_input_manager_push_window (mcw, MNB_INPUT_LAYER_TOP);
    }
  else if (type == META_COMP_WINDOW_DOCK)
    {
      MnbPanel *panel;

      if ((panel = mnb_toolbar_find_panel_for_xid (toolbar, xwin)))
        {
          /*
           * Because of the async nature of the panel dbus API, the following
           * sequence of events is possible (see BMC#2019):
           *
           *   - Request to show a panel,
           *   - Random modal window pops up,
           *   - Hide Toolbar because of the modality,
           *   - Panel window maps,
           *   - We try to show the Panel, this needs Toolbar visible,
           *   - The Toolbar will refuse to show because of the modality,
           *   - This is where it ends, but the Panel window is already mapped,
           *     but because of the animation invisible, obscuring the modal
           *     window.
           *
           *     So, we must check for modal windows here, and if one is
           *     visible, tell the panel to **** off.
           */
          g_signal_handlers_disconnect_by_func (mcw,
                                           meego_netbook_panel_window_show_cb,
                                                plugin);

          if (!meego_netbook_modal_windows_present (plugin, -1))
            {
              /*
               * Order matters; mnb_panel_show_mutter_window() hides the mcw,
               * and since the compositor call clutter_actor_show() when a map
               * effect completes, we have to first let the compositor to do its
               * thing, and only then impose our will on it.
               */
              mutter_plugin_map_completed (plugin, mcw);
              mnb_panel_oop_show_mutter_window (MNB_PANEL_OOP (panel), mcw);
            }
          else
            {
              mutter_plugin_map_completed (plugin, mcw);
              mnb_panel_hide (panel);
            }
        }
      else
        mutter_plugin_map_completed (plugin, mcw);
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
      MetaScreen        *screen = mutter_plugin_get_screen (plugin);
      gint               screen_width, screen_height;
      gfloat             actor_width, actor_height;

      /* Hide toolbar etc in presence of modal window */
      if (meta_window_is_blessed_window (mw))
        mnb_toolbar_hide (MNB_TOOLBAR (priv->toolbar), MNB_SHOW_HIDE_POLICY);

      if (type == META_COMP_WINDOW_NORMAL ||
          type == META_COMP_WINDOW_DIALOG ||
          type == META_COMP_WINDOW_MODAL_DIALOG)
        {
          g_signal_connect (mcw, "window-destroyed",
                            G_CALLBACK (window_destroyed_cb),
                            plugin);
        }

      /*
       * Move application window to a new workspace, if appropriate.
       *
       *  - only move windows when in netbook mode,
       *
       *  - always move windows that set meego-on-new-workspace hint to yes,
       *
       *  - never move windows that set meego-on-new-workspace hint to no,
       *
       *  - apply heuristics when hint is not set:
       *     - only move applications, i.e., _NET_WM_WINDOW_TYPE_NORMAL,
       *     - exclude modal windows and windows that are transient.
       */
      if (priv->netbook_mode)
        {
          MnbThreeState state =
            meego_netbook_mutter_hints_on_new_workspace (mw);

          if (state == MNB_STATE_YES)
            {
              move_window = TRUE;
            }
          else if (state == MNB_STATE_UNSET)
            {
              move_window =
                (type == META_COMP_WINDOW_NORMAL  &&
                 !meta_window_is_modal (mw)       &&
                 meta_window_get_transient_for_as_xid (mw) == None &&
                 !meego_netbook_never_move_window (mw));
            }
        }

      if (move_window)
        {
          MetaDisplay *display = meta_screen_get_display (screen);
          guint32      timestamp;

          timestamp = meta_display_get_current_time_roundtrip (display);

          meego_netbook_move_window_to_its_workspace (plugin,
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
          mutter_plugin_map_completed (plugin, mcw);
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
        mutter_plugin_map_completed (plugin, mcw);
    }
  else
    {
      /*
       * For any other typed windows, connect to the window-destroyed signal.
       */
      g_signal_connect (mcw, "window-destroyed",
                        G_CALLBACK (window_destroyed_cb),
                        plugin);

      mutter_plugin_map_completed (plugin, mcw);
    }
}

static void
destroy (MutterPlugin *plugin, MutterWindow *mcw)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  MetaCompWindowType          type;
  Window                      xwin;

  type = mutter_window_get_window_type (mcw);
  xwin = mutter_window_get_x_window (mcw);

  if (type == META_COMP_WINDOW_DOCK)
    {
      MnbPanel   *panel;
      MnbToolbar *toolbar = MNB_TOOLBAR (priv->toolbar);

      if ((panel = mnb_toolbar_find_panel_for_xid (toolbar, xwin)))
        {
          mnb_panel_oop_hide_animate (MNB_PANEL_OOP (panel), mcw);
          return;
        }
    }

  handle_window_destruction (mcw, plugin);
  mutter_plugin_destroy_completed (plugin, mcw);
}

static void
switch_workspace (MutterPlugin         *plugin,
                  gint                  from,
                  gint                  to,
                  MetaMotionDirection   direction)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;;

  /*
   * We do not run an effect if a panel is visible (as the effect runs above
   * the panel.
   */
  if (mnb_toolbar_get_active_panel (MNB_TOOLBAR (priv->toolbar)))
    {
      mutter_plugin_switch_workspace_completed (plugin);
    }
  else
    {
      mnb_switch_zones_effect (plugin, from, to, direction);
    }
}

static void
last_focus_weak_notify_cb (gpointer data, GObject *meta_win)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (data)->priv;

  if ((MetaWindow*)meta_win == priv->last_focused)
    {
      priv->last_focused = NULL;
    }
}

static gboolean
xevent_filter (MutterPlugin *plugin, XEvent *xev)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;

  if (priv->saver_base && xev->type == priv->saver_base + ScreenSaverNotify)
    {
      XScreenSaverNotifyEvent *sn = (XScreenSaverNotifyEvent*)xev;

      if (sn->state == ScreenSaverOn)
        {
          priv->screen_saver_dpms = TRUE;

          if (!priv->compositor_disabled)
            {
              g_debug ("disabling compositor (DPMS)");
              meego_netbook_toggle_compositor (plugin, FALSE);
            }
        }
      else
        {
          priv->screen_saver_dpms = FALSE;

          if (priv->compositor_disabled && !priv->screen_saver_mcw)
            {
              gboolean on = TRUE;

              if (!(compositor_options & MNB_OPTION_COMPOSITE_FULLSCREEN_APPS))
                on = !meego_netbook_fullscreen_apps_present (plugin);

              if (on)
                {
                  g_debug ("enabling compositor (DPMS)");
                  meego_netbook_toggle_compositor (plugin, on);
                }
            }
        }
    }

  /*
   * Avoid any unnecessary procesing here, as this function is called all the
   * time.
   *
   * We ourselves only need to do extra work for Key events, and the Switcher
   * only cares about Key and Pointer events.
   */
  if (xev->type == KeyPress      ||
      xev->type == KeyRelease    ||
      xev->type == ButtonPress   ||
      xev->type == ButtonRelease ||
      xev->type == MotionNotify)
    {
      MnbAlttabOverlay *overlay;

      overlay = MNB_ALTTAB_OVERLAY (priv->switcher_overlay);

      if (overlay && mnb_alttab_overlay_handle_xevent (overlay, xev))
        return TRUE;

      if (xev->type == KeyPress || xev->type == KeyRelease)
        {
          MetaScreen   *screen = mutter_plugin_get_screen (plugin);
          ClutterActor *stage  = mutter_get_stage_for_screen (screen);
          Window        xwin;

          /*
           * We only get key events on the no-focus window, but for clutter we
           * need to pretend they come from the stage window.
           */
          xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

          xev->xany.window = xwin;
        }
    }

  return (clutter_x11_handle_event (xev) != CLUTTER_X11_FILTER_CONTINUE);
}

static void
kill_switch_workspace (MutterPlugin *plugin)
{
}

static void
kill_window_effects (MutterPlugin *plugin, MutterWindow *mcw)
{
  ActorPrivate *apriv;

  apriv = get_actor_private (mcw);

  if (apriv->tml_minimize)
    {
      clutter_timeline_stop (apriv->tml_minimize);
      g_signal_emit_by_name (apriv->tml_minimize, "completed", NULL);
    }

  if (apriv->tml_maximize)
    {
      clutter_timeline_stop (apriv->tml_maximize);
      g_signal_emit_by_name (apriv->tml_maximize, "completed", NULL);
    }

  if (apriv->tml_map)
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
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  Window                      xwin;
  XSetWindowAttributes        attr;
  Display                    *xdpy    = mutter_plugin_get_xdisplay (plugin);
  MetaScreen                 *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);
  Atom                        type_atom;

  type_atom = meta_display_get_atom (display,
                                     META_ATOM__NET_WM_WINDOW_TYPE_DOCK);

  attr.event_mask        = KeyPressMask | KeyReleaseMask | FocusChangeMask;
  attr.override_redirect = False;

  meta_error_trap_push (display);

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

  meta_error_trap_pop (display, FALSE);

  priv->focus_xwin = xwin;
}

static void
setup_screen_saver (MutterPlugin *plugin)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  Display                    *xdpy    = mutter_plugin_get_xdisplay (plugin);
  MetaScreen                 *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);
  Window                      xroot;

  xroot = RootWindow (xdpy, meta_screen_get_screen_number (screen));

  meta_error_trap_push (display);

  if (XScreenSaverQueryExtension (xdpy, &priv->saver_base, &priv->saver_error))
    XScreenSaverSelectInput (xdpy, xroot, ScreenSaverNotifyMask);

  meta_error_trap_pop (display, FALSE);
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
 * If complete == TRUE the texture is painted in it's entirety, otherwise only
 * parts not occluded by windows are painted (up to a point; if the partial
 * paint require more than MAX_RECTS of separate calls to cogl, the whole
 * texture is painted instead).
 *
 * Returns TRUE if the texture was painted; if FALSE, regular path for
 * painting textures should be fallen back on.
 *
 */
static gboolean
mnb_desktop_texture_paint (ClutterActor *actor,
                           gboolean      complete,
                           MetaScreen   *screen)
{
  static CoglHandle material = COGL_INVALID_HANDLE;

  MutterPlugin               *plugin = meego_netbook_get_plugin_singleton ();
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;

  CoglHandle paint_tex;
  guint tex_width, tex_height;
  gfloat bw, bh;                /* base texture size        */
  gfloat aw, ah;                /* allocation texture size  */
  gfloat vw = 0.5, vh = 0.5;    /* scaled texture half-size */

  ClutterActorBox alloc;
  GdkRegion *visible_region = NULL;
  gboolean retval = TRUE;

  if (!complete)
    {
      visible_region = mnb_get_background_visible_region (screen);

      /*
       * If the visible region is NULL (e.g., effect is running), force
       * painting of the entire desktop.
       */
      if (!visible_region)
        complete = TRUE;
      else if (gdk_region_empty (visible_region))
        goto finish_up;
    }

  if (!CLUTTER_ACTOR_IS_REALIZED (actor))
    clutter_actor_realize (actor);

  paint_tex = clutter_texture_get_cogl_texture (CLUTTER_TEXTURE (actor));

  if (paint_tex == COGL_INVALID_HANDLE)
    goto finish_up;

  tex_width = cogl_texture_get_width (paint_tex);
  tex_height = cogl_texture_get_height (paint_tex);

  if (tex_width == 0 || tex_height == 0) /* no contents yet */
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

  bw = tex_width;
  bh = tex_height;

  aw = alloc.x2 - alloc.x1;
  ah = alloc.y2 - alloc.y1;

  if (priv->scaled_background)
    {
      /*
       * We implement the center, scale without distortion and crop algorithm
       * that is used in PengeMagicTexture.
       */
      if ((float)bw/bh < (float)aw/ah)
        {
          /* fit width */
          vh = (((float)ah * bw) / ((float)aw * bh)) / 2;
        }
      else
        {
          /* fit height */
          vw = (((float)aw * bh) / ((float)ah * bw)) / 2;
        }
    }

  if (complete)
    {
      if (priv->scaled_background)
        {
          gfloat tx1, tx2, ty1, ty2;

          tx1 = (0.5 - vw);
          tx2 = (0.5 + vw);
          ty1 = (0.5 - vh);
          ty2 = (0.5 + vh);

          cogl_rectangle_with_texture_coords (0, 0,
                                              alloc.x2 - alloc.x1,
                                              alloc.y2 - alloc.y1,
                                              tx1, ty1,
                                              tx2, ty2);
          return TRUE;
        }
      else
        return FALSE;
    }
  else if (!gdk_region_empty (visible_region))
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

          if (priv->scaled_background)
            {
              gfloat tx1, tx2, ty1, ty2;

              tx1 = (0.5 - vw);
              tx2 = (0.5 + vw);
              ty1 = (0.5 - vh);
              ty2 = (0.5 + vh);

              cogl_rectangle_with_texture_coords (0, 0,
                                                  alloc.x2 - alloc.x1,
                                                  alloc.y2 - alloc.y1,
                                                  tx1, ty1,
                                                  tx2, ty2);
              return TRUE;
            }

          /*
           * This is the simple case; we return FALSE, allowing the paint to
           * reach the normal texture paint method.
           */
          retval = FALSE;
	}
      else
	{
	  float coords[MAX_RECTS * 8];

          if (priv->scaled_background)
            {
              for (i = 0; i < n_rects; i++)
                {
                  GdkRectangle *rect = &rects[i];

                  /*
                   * The texture coordinates are first normalized for the
                   * allocation size, then scaled by the scaled texture width,
                   * and finally translated by the scaled texture offset (which,
                   * since the texture is centered, is 1/2 - half_size).
                   */
                  gfloat x1 = rect->x / aw;
                  gfloat x2 = (rect->x + rect->width) / aw;
                  gfloat y1 = rect->y / ah;
                  gfloat y2 = (rect->y + rect->height)/ ah;

                  x1 = x1 * 2 * vw + (0.5 - vw);
                  x2 = x2 * 2 * vw + (0.5 - vw);
                  y1 = y1 * 2 * vh + (0.5 - vh);
                  y2 = y2 * 2 * vh + (0.5 - vh);

                  coords[i * 8 + 0] = rect->x;
                  coords[i * 8 + 1] = rect->y;
                  coords[i * 8 + 2] = rect->x + rect->width;
                  coords[i * 8 + 3] = rect->y + rect->height;
                  coords[i * 8 + 4] = x1;
                  coords[i * 8 + 5] = y1;
                  coords[i * 8 + 6] = x2;
                  coords[i * 8 + 7] = y2;
                }
            }
          else
            {
              for (i = 0; i < n_rects; i++)
                {
                  GdkRectangle *rect = &rects[i];

                  coords[i * 8 + 0] = rect->x;
                  coords[i * 8 + 1] = rect->y;
                  coords[i * 8 + 2] = rect->x + rect->width;
                  coords[i * 8 + 3] = rect->y + rect->height;
                  coords[i * 8 + 4] = rect->x / bw;
                  coords[i * 8 + 5] = rect->y / bh;
                  coords[i * 8 + 6] = (rect->x + rect->width)  / bw;
                  coords[i * 8 + 7] = (rect->y + rect->height) / bh;
                }
            }

	  g_free (rects);

	  cogl_rectangles_with_texture_coords (coords, n_rects);
	}
    }

 finish_up:
  gdk_region_destroy (visible_region);
  return retval;
}

/*
 * Avoid painting the desktop background if it is completely occluded.
 */
static void
desktop_background_paint (ClutterActor *background, MutterPlugin *plugin)
{
  MetaScreen *screen;

  /*
   * Don't paint desktop background if fullscreen application is present.
   */
  if (meego_netbook_fullscreen_apps_present (plugin))
    goto finish_up;

  /*
   * Try to paint only parts of the desktop background; however, we are painting
   * on behalf of a clone, force complete paint.
   */
  screen = mutter_plugin_get_screen (plugin);
  if (!mnb_desktop_texture_paint (background,
                                  clutter_actor_is_in_clone_paint (background),
                                  screen))
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
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen = mutter_plugin_get_screen (plugin);
  gint                        screen_width, screen_height;
  ClutterActor               *new_texture;
  ClutterActor               *old_texture = priv->desktop_tex;

  mutter_plugin_query_screen_size (MUTTER_PLUGIN (plugin),
                                   &screen_width, &screen_height);

  g_assert (filename);

  new_texture = (ClutterActor*)
    mx_texture_cache_get_texture (mx_texture_cache_get_default (), filename);

  priv->desktop_tex = new_texture;

  if (old_texture)
    clutter_actor_destroy (old_texture);

  if (!new_texture)
    {
      g_warning ("Failed to load '%s', No tiled desktop image", filename);
    }
  else
    {
      ClutterActor *stage = mutter_get_stage_for_screen (screen);

      if (clutter_texture_get_pixel_format (CLUTTER_TEXTURE (new_texture)) &
          COGL_A_BIT)
        {
          g_warning ("Desktop background '%s' has alpha channel", filename);
        }

      clutter_actor_set_size (new_texture, screen_width, screen_height);

      clutter_texture_set_repeat (CLUTTER_TEXTURE (new_texture), TRUE, TRUE);

      if (new_texture)
        {
          clutter_container_add_actor (CLUTTER_CONTAINER (stage), new_texture);
          clutter_actor_lower_bottom (new_texture);

          g_signal_connect (new_texture, "paint",
                            G_CALLBACK (desktop_background_paint),
                            plugin);
        }
    }
}

static void
desktop_background_changed_cb (GConfClient *client,
                               guint        cnxn_id,
                               GConfEntry  *entry,
                               gpointer     data)
{
  MutterPlugin               *plugin = MUTTER_PLUGIN (data);
  const gchar                *filename = NULL;
  GConfValue                 *value;
  const gchar                *key;

  if (!entry)
    return;

  key   = gconf_entry_get_key (entry);
  value = gconf_entry_get_value (entry);

  if (!strcmp (key, KEY_BG_FILENAME))
    {
      if (value)
        filename = gconf_value_get_string (value);

      if (!filename || !*filename)
        filename = THEMEDIR "/panel/background-tile.png";

      setup_desktop_background (plugin, filename);
    }
#if 0
  else if (value && !strcmp (key, KEY_BG_OPTIONS))
    {
      MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;

      g_return_if_fail (value->type == GCONF_VALUE_STRING);

      const gchar *opt = gconf_value_get_string (value);

      /*
       * Anything other than 'wallpaper' is scaled
       */
      if (strcmp (opt, "wallpaper"))
        priv->scaled_background = TRUE;
      else
        priv->scaled_background = FALSE;
    }
#endif
}

static void
desktop_background_init (MutterPlugin *plugin)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  GError *error = NULL;

  gconf_client_add_dir (priv->gconf_client,
                        BG_KEY_DIR,
                        GCONF_CLIENT_PRELOAD_NONE,
                        &error);

  if (error)
    {
      g_warning (G_STRLOC ": Error when adding directory for notification: %s",
                 error->message);
      g_clear_error (&error);
    }

  gconf_client_notify_add (priv->gconf_client,
                           BG_KEY_DIR,
                           desktop_background_changed_cb,
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
#if 0
  gconf_client_notify (priv->gconf_client, KEY_BG_OPTIONS);
#endif
}

/*
 * Core of the plugin init function, called for initial initialization and
 * by the reload() function. Returns TRUE on success.
 */
static const MutterPluginInfo *
plugin_info (MutterPlugin *plugin)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;

  return &priv->info;
}

static void
focus_xwin (MutterPlugin *plugin, guint xid)
{
  MetaDisplay *display;

  display = meta_screen_get_display (mutter_plugin_get_screen (plugin));

  meta_error_trap_push (display);

  XSetInputFocus (meta_display_get_xdisplay (display), xid,
                  RevertToPointerRoot, CurrentTime);

  meta_error_trap_pop (display, TRUE);
}

void
meego_netbook_stash_window_focus (MutterPlugin *plugin, guint32 timestamp)
{
  MeegoNetbookPluginPrivate *priv    = MEEGO_NETBOOK_PLUGIN (plugin)->priv;

  if (timestamp == CurrentTime)
    timestamp = clutter_x11_get_current_event_time ();

  focus_xwin (plugin, priv->focus_xwin);
}

void
meego_netbook_unstash_window_focus (MutterPlugin *plugin, guint32 timestamp)
{
  MeegoNetbookPluginPrivate *priv    = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen                 *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay                *display = meta_screen_get_display (screen);
  MetaWindow                 *focus;
  MnbPanel                   *panel;

  /*
   * First check if a panel is active; if it's an OOP panel, we focus the panel,
   * if it's the Switcher, we unstash normally.
   */
  panel = mnb_toolbar_get_active_panel (MNB_TOOLBAR (priv->toolbar));

  if (panel)
    {
      if (MNB_IS_PANEL_OOP (panel))
        {
          mnb_panel_oop_focus ((MnbPanelOop*)panel);
          return;
        }
    }

  if (timestamp == CurrentTime)
    timestamp = meta_display_get_current_time_roundtrip (display);

  /*
   * Work out what we should focus next.
   *
   * First, we try the window the WM last requested focus for; if this is
   * not available, we try the last focused window.
   */
  focus = meta_display_get_expected_focus_window (display);

  if (!focus)
    focus = priv->last_focused;

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

MutterPlugin *
meego_netbook_get_plugin_singleton (void)
{
  return plugin_singleton;
}

/*
 * Returns TRUE if a top-level modal window is present on given workspace; if
 * workspace is < 0, returns TRUE if any modal windows are present on any
 * workspace.
 */
gboolean
meego_netbook_modal_windows_present (MutterPlugin *plugin, gint workspace)
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
      if (meta_window_is_modal (w) &&
          meta_window_get_transient_for_as_xid (w) == None)
        {
          if (workspace < 0)
            {
              return TRUE;
            }
          else
            {
              gint s = mutter_window_get_workspace (m);

              if (s < 0 || s == workspace)
                return TRUE;
            }
        }

      l = l->next;
     }

  return FALSE;
}

gboolean
meego_netbook_compositor_disabled (MutterPlugin *plugin)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;

  return priv->compositor_disabled;
}

void
meego_netbook_activate_window (MetaWindow *window)
{
  MetaWorkspace *workspace;
  MetaWorkspace *active_workspace;
  MetaScreen    *screen;
  MetaDisplay   *display;
  guint32        timestamp;

  screen           = meta_window_get_screen (window);
  display          = meta_screen_get_display (screen);
  workspace        = meta_window_get_workspace (window);
  active_workspace = meta_screen_get_active_workspace (screen);

  /*
   * Make sure our stamp is recent enough.
   */
  timestamp = meta_display_get_current_time_roundtrip (display);

  if (!active_workspace || (active_workspace == workspace))
    {
      meta_window_activate_with_workspace (window, timestamp, workspace);
    }
  else
    {
      meta_workspace_activate_with_focus (workspace, window, timestamp);
    }
}

ClutterActor *
meego_netbook_get_toolbar (MutterPlugin *plugin)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;

  return priv->toolbar;
}

gboolean
meego_netbook_activate_mutter_window (MutterWindow *mcw)
{
  MetaWindow     *window;
  MetaWorkspace  *workspace;
  MetaWorkspace  *active_workspace;
  MetaScreen     *screen;
  MetaDisplay    *display;
  guint32         timestamp;

  window           = mutter_window_get_meta_window (mcw);
  screen           = meta_window_get_screen (window);
  display          = meta_screen_get_display (screen);
  workspace        = meta_window_get_workspace (window);
  active_workspace = meta_screen_get_active_workspace (screen);

  /*
   * Make sure our stamp is recent enough.
   */
  timestamp = meta_display_get_current_time_roundtrip (display);

  if (!active_workspace || (active_workspace == workspace))
    {
      meta_window_activate_with_workspace (window, timestamp, workspace);
    }
  else
    {
      meta_workspace_activate_with_focus (workspace, window, timestamp);
    }

  return TRUE;
}

gboolean
meego_netbook_use_netbook_mode (MutterPlugin *plugin)
{
  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;

  return priv->netbook_mode;
}

guint32
meego_netbook_get_compositor_option_flags (void)
{
  return compositor_options;
}

gboolean
meego_netbook_urgent_notification_present (MutterPlugin *plugin)
{
  return ntf_overlay_urgent_notification_present ();
}

/*
 * pass -1 for any values not to be used.
 */
void
meego_netbook_set_struts (MutterPlugin *plugin,
                           gint          left,
                           gint          right,
                           gint          top,
                           gint          bottom)
{
  MeegoNetbookPluginPrivate *ppriv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  MetaScreen        *screen         = mutter_plugin_get_screen (plugin);
  MetaDisplay       *display        = meta_screen_get_display (screen);
  Display           *xdpy           = meta_display_get_xdisplay (display);
  Window             xwin           = ppriv->focus_xwin;
  Atom               strut_atom     = meta_display_get_atom (display,
                                                      META_ATOM__NET_WM_STRUT);

  static gint32 struts[4] = {-1, -1, -1, -1};

  if (left   != struts[0] ||
      right  != struts[1] ||
      top    != struts[2] ||
      bottom != struts[3])
    {
      if (left >= 0)
        struts[0] = left;

      if (right >= 0)
        struts[1] = right;

      if (top >= 0)
        struts[2] = top;

      if (bottom >= 0)
        struts[3] = bottom;

      if (struts[0] <= 0 &&
          struts[1] <= 0 &&
          struts[2] <= 0 &&
          struts[3] <= 0)
        {
          struts[0] = struts[1] = struts[2] = struts[3] = -1;

          meta_error_trap_push (display);

          XDeleteProperty (xdpy, xwin, strut_atom);

          meta_error_trap_pop (display, FALSE);
        }
      else
        {
          gint32 new_struts[4] = {0,};

          if (struts[0] > 0)
            new_struts[0] = struts[0];

          if (struts[1] > 0)
            new_struts[1] = struts[1];

          if (struts[2] > 0)
            new_struts[2] = struts[2];

          if (struts[3] > 0)
            new_struts[3] = struts[3];

          meta_error_trap_push (display);

          XChangeProperty (xdpy, xwin,
                           strut_atom,
                           XA_CARDINAL, 32, PropModeReplace,
                           (unsigned char *) &new_struts, 4);

          meta_error_trap_pop (display, FALSE);
        }
    }
}
