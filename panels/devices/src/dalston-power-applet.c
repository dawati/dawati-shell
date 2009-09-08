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


#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libnotify/notify.h>

#include <dalston/dalston-volume-applet.h>
#include <dalston/dalston-power-applet.h>
#include <dalston/dalston-button-monitor.h>

#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-panel-gtk.h>
#include <config.h>

#define PADDING 4

#define PKG_THEMEDIR PKG_DATADIR"/theme"

static void
_battery_monitor_status_changed_cb (DalstonBatteryMonitor *monitor,
                                    gpointer               userdata)
{
  GtkWidget *pane;
  GtkWidget *window;
  DalstonPowerApplet *power_applet;
  MplPanelClient *panel_client;

  if (dalston_battery_monitor_get_is_ready (monitor))
  {
    /* We must check if we have an AC adapter, if not then we are not on a
     * portable device and should thus not show the UI
     */

    if (dalston_battery_monitor_get_has_ac_adapter (monitor))
    {
      panel_client = mpl_panel_gtk_new (MPL_PANEL_POWER,
                                        _("power & brightness"),
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

      g_signal_handlers_disconnect_by_func (monitor,
                                            _battery_monitor_status_changed_cb,
                                            userdata);
    }
  }
}

int
main (int    argc,
      char **argv)
{
  GtkSettings *settings;
  DalstonButtonMonitor *button_monitor;
  DalstonBatteryMonitor *battery_monitor;

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

  battery_monitor = g_object_new (DALSTON_TYPE_BATTERY_MONITOR,
                                  NULL);
  g_signal_connect (battery_monitor,
                    "status-changed",
                    (GCallback)_battery_monitor_status_changed_cb,
                    NULL);

  gtk_main ();
}
