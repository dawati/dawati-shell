/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
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

/*
 * Simple test for out of process panels.
 *
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "../libmnb/mnb-panel-client.h"

static void
set_size_cb (MnbPanelClient *panel, guint width, guint height, gpointer data)
{
  GtkWidget *window = GTK_WIDGET (data);

  g_debug ("Setting panel window size to %dx%d", width, height);

  gtk_window_resize (GTK_WINDOW (window), width, height);
}

/*
 * This is a callback to demonstrate how the application can close the config
 * window; however, you probably should not do that in your applicaiton, as
 * the hiding of the config window is taken care of by the mobling UI.
 */
static void
button_clicked_cb (GtkButton *button, gpointer data)
{
  MnbPanelClient *panel = MNB_PANEL_CLIENT (data);

  mnb_panel_client_request_hide (panel);
}

/*
 * Function to create the content of the panel window.
 *
 * This is to demonstrate how the contents can be created dynamically when the
 * window is getting shown. If your config window has static content, you
 * can construct it when creating the GtkPlug (i.e., in this example that would
 * be in the main() function.
 *
 * This function is hooked into the "embedded" signal on the GtkPlug you create.
 */
static void
make_window_content (GtkPlug *window)
{
  GtkWidget *table, *button, *old_child;

  table = gtk_table_new (3, 3, TRUE);

  button = gtk_button_new_from_stock (GTK_STOCK_QUIT);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (button_clicked_cb), window);

  gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 1, 2);

  old_child = gtk_bin_get_child (GTK_BIN (window));

  if (old_child)
    gtk_container_remove (GTK_CONTAINER (window), old_child);

  gtk_widget_show_all (table);

  gtk_container_add (GTK_CONTAINER (window), table);
}

static void
window_embedded_cb (GtkPlug *window, GParamSpec *pspec, gpointer data)
{
  gboolean embedded;

  g_object_get (window, "embedded", &embedded, NULL);
  g_debug ("Embedded: %s", embedded ? "yes" : "no");

  if (embedded)
    make_window_content (window);
}


int
main (int argc, char *argv[])
{
  GtkWidget      *window;
  GtkWidget      *label;
  MnbPanelClient *panel;

  gtk_init (&argc, &argv);

  window = gtk_plug_new (0);

  /*
   * If the content of the config window needs to be created dynamically
   * each time the window is shown, hook into the "embedded" signal on the
   * plug.
   *
   * If your config window has static content, you can just create it here and
   * pack it directly into the plug.
   */
  g_signal_connect (window, "notify::embedded",
                    G_CALLBACK (window_embedded_cb), NULL);

  gtk_widget_show (window);

  panel = mnb_panel_client_new ("/com/intel/Mnb/TestPanel",
                                GDK_WINDOW_XID (window->window),
                                "spaces-people",
                                "people");

  g_signal_connect (panel, "set-size", G_CALLBACK (set_size_cb), window);

  gtk_main ();

  return 0;
}
