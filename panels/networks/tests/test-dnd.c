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

#include <carrick/carrick-list.h>
#include <gtk/gtk.h>

int
main (int argc,
      char **argv)
{
  GtkWidget *window;
  GtkWidget *item;
  GtkWidget *list;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window,
                    "delete-event",
                    (GCallback) gtk_main_quit,
                    NULL);

  list = carrick_list_new ();
  gtk_container_add (GTK_CONTAINER (window),
                     list);

  for (int i = 0; i < 10; i++)
  {
    const gchar *lbl = g_strdup_printf ("Item%i", i);
    item = gtk_button_new_with_label (lbl);
  }

  gtk_widget_set_size_request (window,
    450,
    450);
  gtk_widget_show_all (window);

  gtk_main ();
}
