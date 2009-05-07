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
map_window_cb (MnbPanelClient *panel, guint width, guint height, gpointer data)
{
  GtkWidget *window = GTK_WIDGET (data);

  gtk_window_resize (GTK_WINDOW (window), width, height);
  gtk_widget_show (window);
}

int
main (int argc, char *argv[])
{
  GtkWidget      *window;
  GtkWidget      *label;
  MnbPanelClient *panel;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated (GTK_WINDOW (window), FALSE);

  gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);

  label = gtk_label_new ("Test panel (gtk based).");
  gtk_widget_show (label);

  gtk_container_add (GTK_CONTAINER (window), label);

  gtk_widget_realize (window);

  panel = mnb_panel_client_new ("/com/intel/Mnb/TestPanel",
                                GDK_WINDOW_XID (window->window),
                                "spaces-people",
                                "people");

  g_signal_connect (panel, "map-window", G_CALLBACK (map_window_cb), window);

  gtk_main ();

  return 0;
}
