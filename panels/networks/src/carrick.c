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

#include <carrick/carrick-applet.h>
#include <carrick/carrick-pane.h>
#include <carrick/carrick-status-icon.h>
#include "moblin-netbook-system-tray.h"

#include <config.h>

#define PADDING 4

static void
_plug_notify_embedded (GObject    *object,
                       GParamSpec *pspec,
                       gpointer    user_data)
{
  CarrickApplet *applet = CARRICK_APPLET (user_data);
  CarrickStatusIcon *icon = CARRICK_STATUS_ICON (carrick_applet_get_icon (applet));
  CarrickPane *pane = CARRICK_PANE (carrick_applet_get_pane (applet));

  gboolean embedded;

  g_object_get (object,
                "embedded",
                &embedded,
                NULL);

  carrick_status_icon_set_active (icon, embedded);
  carrick_pane_trigger_scan (pane);
}

int
main (int    argc,
      char **argv)
{
  CarrickApplet *applet;
  GtkWidget     *icon;
  GtkWidget     *pane;
  GdkScreen     *screen;
  GtkWidget     *plug;
  GtkSettings   *settings;

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_init (&argc, &argv);

  /* Force to correct theme */
  settings = gtk_settings_get_default ();
  gtk_settings_set_string_property (settings,
                                    "gtk-theme-name",
                                    "Moblin-Netbook",
                                    NULL);

  applet = carrick_applet_new ();
  icon = carrick_applet_get_icon (applet);
  plug = gtk_plug_new (0);
  g_signal_connect (plug,
                    "notify::embedded",
                    G_CALLBACK (_plug_notify_embedded),
                    applet);

  pane = carrick_applet_get_pane (applet);
  gtk_container_add (GTK_CONTAINER (plug),
                     pane);
  mnbk_system_tray_init (GTK_STATUS_ICON (icon),
                         GTK_PLUG (plug),
                         "wifi");
  screen = gtk_widget_get_screen (plug);
  gtk_widget_set_size_request (pane,
                               gdk_screen_get_width (screen) - 2 * PADDING,
                               450);

  gtk_main ();
}
