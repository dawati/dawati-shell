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

  /*
   * Whatever you do, do not destroy the config window; if you need to replace
   * it with a different one, you will need to run mnbk_system_tray_init()
   * again.
   */
  gtk_widget_hide (config);
}

static void
make_menu_content (GtkPlug *config)
{
  GtkWidget *table, *button, *old_child;

  table = gtk_table_new (3, 3, TRUE);

  button = gtk_button_new_from_stock (GTK_STOCK_QUIT);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (button_clicked_cb), config);

  gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 1, 2);

  old_child = gtk_bin_get_child (GTK_BIN (config));

  if (old_child)
    gtk_container_remove (GTK_CONTAINER (config), old_child);

  gtk_widget_show_all (table);

  gtk_container_add (GTK_CONTAINER (config), table);
}

static void
config_embedded_cb (GtkPlug *config, gpointer data)
{
  gboolean embedded;

  g_object_get (config, "embedded", &embedded, NULL);

  if (embedded)
    make_menu_content (config);
}

int
main (int argc, char *argv[])
{
  GtkWidget     *config;
  GtkStatusIcon *icon;

  gtk_init (&argc, &argv);

  config = gtk_plug_new (0);

  g_signal_connect (config, "embedded", G_CALLBACK (config_embedded_cb), NULL);

  icon = gtk_status_icon_new_from_stock (GTK_STOCK_INFO);

  mnbk_system_tray_init (icon, GTK_PLUG (config));

  gtk_main ();

  return 0;
}
