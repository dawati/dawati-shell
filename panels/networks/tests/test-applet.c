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
#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>


#include <carrick/carrick-applet.h>

int
main (int    argc,
      char **argv)
{
  GtkWidget     *window;
  CarrickApplet *applet;

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "FUT-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window,
                    "delete-event",
                    (GCallback) gtk_main_quit,
                    NULL);

  applet = carrick_applet_new ();
  gtk_container_add (GTK_CONTAINER (window),
                     carrick_applet_get_pane (CARRICK_APPLET (applet)));

  gtk_widget_set_size_request (GTK_WIDGET (window),
                               1024,
                               450);
  gtk_widget_show_all (window);

  gtk_main ();
}
