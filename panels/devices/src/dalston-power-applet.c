/*
 * Dalston - power and volume applets for Moblin
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


#include <X11/XF86keysym.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libnotify/notify.h>

#include <dalston/dalston-volume-applet.h>
#include <dalston/dalston-power-applet.h>
#include <dalston/dalston-button-monitor.h>
#include <dalston/dalston-idle-manager.h>
#include <dalston/dalston-brightness-manager.h>

#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-panel-gtk.h>
#include <config.h>

#define PADDING 4

#define PKG_THEMEDIR PKG_DATADIR"/theme"

static guint _battery_key_code = 0;
static gboolean _panel_is_visible = FALSE;

static gboolean
window_grab_key (GdkWindow *window,
                 guint      key_code)
{
  Display *dpy = GDK_DISPLAY ();
  guint    mask = AnyModifier;
  gint     ret;

  gdk_error_trap_push ();

  ret = XGrabKey (dpy, key_code, mask, GDK_WINDOW_XID (window), True,
                  GrabModeAsync, GrabModeAsync);
  if (BadAccess == ret)
  {
    g_warning ("%s: 'BadAccess' grabbing key %d",
               G_STRLOC,
               key_code);
    return FALSE;
  }

  /* grab the lock key if possible */
  ret = XGrabKey (dpy, key_code, LockMask | mask, GDK_WINDOW_XID (window), True,
                  GrabModeAsync, GrabModeAsync);
  if (BadAccess == ret)
  {
    g_warning ("%s: 'BadAccess' grabbing key %d with LockMask",
               G_STRLOC,
               key_code);
    return FALSE;
  }

  gdk_flush ();
  gdk_error_trap_pop ();

  return TRUE;
}

static GdkFilterReturn
_event_filter_cb (XEvent          *xev,
                  GdkEvent        *ev,
                  MplPanelClient  *panel_client)
{
  if (xev->type == KeyPress &&
      xev->xkey.keycode == _battery_key_code)
  {
    g_debug ("%s() toggle visibility to %d", __FUNCTION__, !_panel_is_visible);
    if (_panel_is_visible)
      mpl_panel_client_request_hide (panel_client);
    else
      mpl_panel_client_request_show (panel_client);
    return GDK_FILTER_REMOVE;
  }

  return GDK_FILTER_CONTINUE;
}

static void
_bind_battery_key (MplPanelClient *panel_client)
{
  GdkWindow *root_window;

  _battery_key_code = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_Battery);
  if (_battery_key_code)
  {
    root_window = gdk_screen_get_root_window (gdk_screen_get_default ());
    window_grab_key (root_window, _battery_key_code);
    gdk_window_add_filter (root_window,
                           (GdkFilterFunc) _event_filter_cb,
                           panel_client);
  } else {
    g_warning ("%s : XKeysymToKeycode (%x) failed. "
               "Battery shortcut not enabled.",
               G_STRLOC,
               XF86XK_Battery);
  }
}

static gboolean
_release_battery_key_cb (GtkWidget       *widget,
                         GdkEvent        *event,
                         MplPanelClient  *panel_client)
{
  GdkWindow *root_window;

  root_window = gdk_screen_get_root_window (gdk_screen_get_default ());
  gdk_window_remove_filter (root_window,
                            (GdkFilterFunc) _event_filter_cb,
                            panel_client);

  XUngrabKey (GDK_DISPLAY (), _battery_key_code,
              AnyModifier, GDK_WINDOW_XID (root_window));
  XUngrabKey (GDK_DISPLAY (), _battery_key_code,
              AnyModifier | LockMask, GDK_WINDOW_XID (root_window));

  return FALSE;
}

static void
_panel_show_begin_cb (MplPanelClient  *panel_client,
                      gpointer         data)
{
  _panel_is_visible = TRUE;
}

static void
_panel_hide_begin_cb (MplPanelClient  *panel_client,
                      gpointer         data)
{
  _panel_is_visible = FALSE;
}

static void
_setup_panel (DalstonBatteryMonitor *monitor)
{
  GtkWidget *pane;
  GtkWidget *window;
  DalstonPowerApplet *power_applet;
  MplPanelClient *panel_client;

  panel_client = mpl_panel_gtk_new (MPL_PANEL_POWER,
                                    _("battery"),
                                    PKG_THEMEDIR "/power-applet.css",
                                    "unknown",
                                    TRUE);
  mpl_panel_client_set_height_request (panel_client, 200);

  /* Power applet */
  power_applet = dalston_power_applet_new (panel_client,
                                           monitor);

  window = mpl_panel_gtk_get_window (MPL_PANEL_GTK (panel_client));
  pane = dalston_power_applet_get_pane (power_applet);
  gtk_container_add (GTK_CONTAINER (window), pane);
  gtk_widget_show (window);

  /* Bind to battery key. */
  _bind_battery_key (panel_client);
  g_signal_connect (window, "delete-event",
                    G_CALLBACK (_release_battery_key_cb), panel_client);

  g_signal_connect (panel_client, "show-begin",
                    G_CALLBACK (_panel_show_begin_cb), NULL);
  g_signal_connect (panel_client, "hide-begin",
                    G_CALLBACK (_panel_hide_begin_cb), NULL);
}

static void
_battery_monitor_status_changed_cb (DalstonBatteryMonitor *monitor,
                                    gpointer               userdata)
{


  if (dalston_battery_monitor_get_is_ready (monitor))
  {
    /* We must check if we have an AC adapter, if not then we are not on a
     * portable device and should thus not show the UI
     */

    if (dalston_battery_monitor_get_has_ac_adapter (monitor))
    {
      _setup_panel (monitor);
    }

    g_signal_handlers_disconnect_by_func (monitor,
                                          _battery_monitor_status_changed_cb,
                                          userdata);
  }
}

int
main (int    argc,
      char **argv)
{
  GtkSettings *settings;
  DalstonButtonMonitor *button_monitor;
  DalstonBatteryMonitor *battery_monitor;
  DalstonIdleManager *idle_manager;
  DalstonBrightnessManager *brightness_manager;

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_init (&argc, &argv);
  notify_init ("Dalston Power Applet");

  /* Monitor buttons */
  button_monitor = dalston_button_monitor_new ();

  /* Force to the moblin theme (for testing) */
  settings = gtk_settings_get_default ();
  gtk_settings_set_string_property (settings,
                                    "gtk-theme-name",
                                    "Moblin-Netbook",
                                    NULL);


  idle_manager = dalston_idle_manager_new ();

  brightness_manager = dalston_brightness_manager_dup_singleton ();
  dalston_brightness_manager_maximise (brightness_manager);

  battery_monitor = g_object_new (DALSTON_TYPE_BATTERY_MONITOR,
                                  NULL);

  if (dalston_battery_monitor_get_is_ready (battery_monitor) &&
      dalston_battery_monitor_get_has_ac_adapter (battery_monitor))

  {
    _setup_panel (battery_monitor);
  } else {
    g_signal_connect (battery_monitor,
                      "status-changed",
                      (GCallback)_battery_monitor_status_changed_cb,
                      NULL);
  }

  gtk_main ();
}
