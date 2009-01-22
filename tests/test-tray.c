/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
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
 * Simple system tray test.
 */

#include "../src/moblin-netbook-system-tray.h"

static void
button_clicked_cb (GtkButton *button, gpointer data)
{
  GtkWidget *config = data;

  gtk_widget_hide (config);
}

/*
 * Re-enable showing of the widget once we are removed from the socket.
 */
static gboolean
plug_show_on_delete (GtkWidget *widget, gpointer data)
{
  gtk_widget_show_all (widget);
  return TRUE;
}

int
main (int argc, char *argv[])
{
  GtkWidget     *config, *button, *table;
  GtkStatusIcon *icon;

  gtk_init (&argc, &argv);

  table = gtk_table_new (3, 3, TRUE);

  config = gtk_plug_new (0);

  /*
   * When we hide the config, the tray manager will remove the window
   * from it's container; this triggers the delete-event; we want to (a) stop
   * this event from destroying our plug (so we can show it again) and (b)
   * call _show_all() on the plug, to make sure that when it is needed again
   * it can be made visible by the socket.
   */
  g_signal_connect (config, "delete-event",
                    G_CALLBACK (plug_show_on_delete), NULL);

  gtk_container_add (GTK_CONTAINER (config), table);

  button = gtk_button_new_from_stock (GTK_STOCK_QUIT);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (button_clicked_cb), config);

  gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 1, 2);

  icon = gtk_status_icon_new_from_stock (GTK_STOCK_INFO);

  mnbk_system_tray_init (icon, GTK_PLUG (config));

  gtk_main ();

  return 0;
}
