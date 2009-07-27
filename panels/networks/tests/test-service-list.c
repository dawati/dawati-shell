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
#include <gconnman/gconnman.h>
#include <carrick/carrick-list.h>
#include <carrick/carrick-service-item.h>

void
add_services (gpointer data,
              gpointer user_data)
{
  GtkWidget *item;

  item = carrick_service_item_new (CM_SERVICE (data));
  carrick_list_add_item (CARRICK_LIST (user_data),
                         item);
}

int
main (int argc,
      char **argv)
{
  GtkWidget *window;
  GtkWidget *service_list;
  CmManager *manager;
  GError *error;
  GList *services;
  GtkSettings *settings;

  gtk_init (&argc, &argv);

  settings = gtk_settings_get_default ();
  gtk_settings_set_string_property (settings,
                                    "gtk-theme-name",
                                    "Moblin-Netbook",
                                    NULL);

  manager = cm_manager_new (&error, FALSE);
  if (error)
  {
    g_debug ("Oh dear, management error: %s",
             error->message);
    g_clear_error (&error);
    return -1;
  }

  services = cm_manager_get_services (manager);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window,
                    "delete-event",
                    (GCallback) gtk_main_quit,
                    NULL);
  service_list = carrick_list_new ();
  gtk_container_add ((GtkContainer *)window,
                     service_list);

  g_list_foreach (services,
                  add_services,
                  (gpointer) service_list);

  gtk_widget_show_all (window);

  gtk_main ();

  g_list_free (services);
}
