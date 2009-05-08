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

#include "../libmnb/mnb-panel-gtk.h"

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
 * the hiding of the config window is taken care of by the Moblin UI.
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
 * window is getting shown. If your window has static content, you can construct
 * it immediately after creating the panel object.
 */
static void
make_window_content (GtkWidget *window)
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
  gtk_widget_show (window);
}

static void
update_content_cb (MnbPanelGtk *panel, gpointer data)
{
  g_debug ("Making content\n");
  make_window_content (mnb_panel_gtk_get_window (panel));
}


int
main (int argc, char *argv[])
{
  GtkWidget      *window;
  GtkWidget      *label;
  MnbPanelClient *panel;

  gtk_init (&argc, &argv);

  panel = mnb_panel_gtk_new ("/com/intel/Mnb/TestPanel",
                             "people-zone",
                             "people");

  g_signal_connect (panel, "show-begin",
                    G_CALLBACK (update_content_cb), NULL);

  gtk_main ();

  return 0;
}
