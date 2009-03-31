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

/*
 * This is a callback to demonstrate how the application can close the config
 * window; however, you probably should not do that in your applicaiton, as
 * the hiding of the config window is taken care of by the mobling UI.
 */
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

/*
 * Function to create the content of the config window.
 *
 * This is to demonstrate how the contents can be created dynamically when the
 * window is getting shown. If your config window has static content, you
 * can construct it when creating the GtkPlug (i.e., in this example that would
 * be in the main() function.
 *
 * This function is hooked into the "embedded" signal on the GtkPlug you create.
 */
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

/*
 * The embedded signal on the GtkPlug can be used to construct the contents
 * of the config window when the window is getting shown.
 */
static void
config_embedded_cb (GtkPlug    *config,
                    GParamSpec *pspec,
                    gpointer    data)
{
  gboolean embedded;

  g_object_get (config, "embedded", &embedded, NULL);
  g_debug ("Embedded: %s", embedded ? "yes" : "no");

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

  /*
   * If the content of the config window needs to be created dynamically
   * each time the window is shown, hook into the "embedded" signal on the
   * plug.
   *
   * If your config window has static content, you can just create it here and
   * pack it directly into the plug.
   */
  g_signal_connect (config, "notify::embedded", G_CALLBACK (config_embedded_cb), NULL);

  icon = gtk_status_icon_new_from_stock (GTK_STOCK_INFO);

  mnbk_system_tray_init (icon, GTK_PLUG (config), "test");

  gtk_main ();

  return 0;
}
