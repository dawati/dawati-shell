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
#include "moblin-netbook-system-tray.h"

#include <config.h>

#define PADDING 4

static void
_plug_notify_embedded (GObject    *object,
                       GParamSpec *pspec,
                       gpointer    user_data)
{
  CarrickApplet *applet = CARRICK_APPLET (user_data);
  CarrickPane *pane = CARRICK_PANE (carrick_applet_get_pane (applet));

  gboolean embedded;

  g_object_get (object,
                "embedded",
                &embedded,
                NULL);

  if (embedded)
  {
    carrick_pane_update (pane);
  }
}

int
main (int    argc,
      char **argv)
{
  CarrickApplet *applet;
  GtkWidget     *pane;
  GdkScreen     *screen;
  GtkWidget     *plug;
  GtkSettings   *settings;
  gboolean       standalone = FALSE;
  GError        *error = NULL;
  GOptionEntry   entries[] = {
    { "standalone", 's', 0, G_OPTION_ARG_NONE, &standalone,
      _("Run in standalone mode"), NULL },
    { NULL }
  };

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  g_set_application_name (_("Carrick connectivity applet"));
  gtk_init_with_args (&argc, &argv, _("- Moblin connectivity applet"),
                      entries, GETTEXT_PACKAGE, &error);

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
    plug = gtk_plug_new (0);
    g_signal_connect (plug,
                      "notify::embedded",
                      G_CALLBACK (_plug_notify_embedded),
                      applet);

    gtk_container_add (GTK_CONTAINER (plug),
                       pane);
    mnbk_system_tray_init (NULL,
                           GTK_PLUG (plug),
                           "wifi");
    screen = gtk_widget_get_screen (plug);
    gtk_widget_set_size_request (pane,
                                 gdk_screen_get_width (screen) - 2 * PADDING,
                                 450);
  }
  else
  {
    plug = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (plug,
                      "delete-event",
                      (GCallback) gtk_main_quit,
                      NULL);
    gtk_container_add (GTK_CONTAINER (plug),
                       pane);

    gtk_widget_show (plug);
  }

  gtk_main ();
}
