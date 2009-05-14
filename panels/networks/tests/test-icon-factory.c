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

#include "carrick-icon-factory.h"
#include <gconnman/gconnman.h>

int
main (int    argc,
      char **argv)
{
  CarrickIconFactory *icon_factory;
  GtkWidget *window;
  GtkWidget *image;
  CmManager *manager;
  CmService *service;
  GError *error = NULL;
  GdkPixbuf *pixbuf;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window,
                    "delete-event",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  icon_factory = carrick_icon_factory_new ();
  manager = cm_manager_new (&error);
  if (error)
  {
    g_debug ("Error initialising connman manager: %s\n",
             error->message);
    g_clear_error (&error);
    return -1;
  }
  cm_manager_refresh (manager);
  service = cm_manager_get_active_service (manager);

  g_print ("NULL service icon is :%s\n",
           carrick_icon_factory_get_path_for_service(service));

  pixbuf = carrick_icon_factory_get_pixbuf_for_service (icon_factory,
                                                       NULL);
  image = gtk_image_new_from_pixbuf (pixbuf);
  gtk_container_add (GTK_CONTAINER (window),
                     image);

  gtk_widget_set_size_request (window,
                               50,
                               50);
  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
