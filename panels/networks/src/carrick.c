/*
 * Carrick - a connection panel for the Moblin Netbook
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Written by - Joshua Lock <josh@linux.intel.com>
 *
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>
#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-panel-gtk.h>

#include <config.h>

#include "carrick/carrick-applet.h"
#include "carrick/carrick-pane.h"

#define PKG_THEME_DIR PKG_DATA_DIR"/theme"

static void
_client_set_size_cb (MplPanelClient *client,
                     guint           width,
                     guint           height,
                     gpointer        user_data)
{
  CarrickApplet *applet = CARRICK_APPLET (user_data);
  CarrickPane   *pane = CARRICK_PANE (carrick_applet_get_pane (applet));

  carrick_pane_update (pane);
}

static void
_connection_changed_cb (CarrickPane     *pane,
                        const gchar     *connection_type,
                        guint            strength,
                        MplPanelClient  *panel_client)
{
  CarrickIconState   icon_state;
  const gchar       *icon_id;

  icon_state = carrick_icon_factory_get_state (connection_type, strength);
  icon_id = carrick_icon_factory_get_name_for_state (icon_state);

  mpl_panel_client_request_button_style (panel_client, icon_id);
}

int
main (int    argc,
      char **argv)
{
  CarrickApplet *applet;
  GtkWidget     *window;
  GtkWidget     *pane;
  GdkScreen     *screen;
  GtkSettings   *settings;
  gboolean       standalone = FALSE;
  GError        *error = NULL;
  GOptionEntry   entries[] = {
    { "standalone", 's', 0, G_OPTION_ARG_NONE, &standalone,
      _ ("Run in standalone mode"), NULL },
    { NULL }
  };

  if (!g_thread_supported ())
    g_thread_init (NULL);

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  g_set_application_name (_ ("Carrick connectivity applet"));
  gtk_init_with_args (&argc, &argv, _ ("- Moblin connectivity applet"),
                      entries, GETTEXT_PACKAGE, &error);
  dbus_g_thread_init ();

  if (error)
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      return 1;
    }

  /* Force to correct theme */
  settings = gtk_settings_get_default ();
  gtk_settings_set_string_property (settings,
                                    "gtk-theme-name",
                                    "Moblin-Netbook",
                                    NULL);

  applet = carrick_applet_new ();
  pane = carrick_applet_get_pane (applet);
  if (!standalone)
    {
      MplPanelClient *panel_client = mpl_panel_gtk_new (MPL_PANEL_NETWORK,
                                                        _("sound"),
                                                        PKG_THEME_DIR "/network-applet.css", /* TODO robsta */
                                                        "unknown",
                                                        TRUE);
      g_signal_connect (panel_client,
                        "set-size",
                        (GCallback) _client_set_size_cb,
                        applet);
      mpl_panel_client_set_height_request (panel_client, 450);
      window = mpl_panel_gtk_get_window (MPL_PANEL_GTK (panel_client));
      gtk_container_add (GTK_CONTAINER (window), pane);

      g_signal_connect (pane,
                        "connection-changed",
                        (GCallback) _connection_changed_cb,
                        panel_client);

      gtk_widget_show_all (window);
    }
  else
    {
      window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      g_signal_connect (window,
                        "delete-event",
                        (GCallback) gtk_main_quit,
                        NULL);
      gtk_container_add (GTK_CONTAINER (window),
                         pane);

      gtk_widget_show (window);
    }

  gtk_main ();
}
