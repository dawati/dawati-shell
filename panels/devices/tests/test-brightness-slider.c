/*
 * Dalston - power and volume applets for Moblin
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


#include <dalston/dalston-brightness-manager.h>
#include <dalston/dalston-brightness-slider.h>

#include <gtk/gtk.h>

int
main (int    argc,
      char **argv)
{
  DalstonBrightnessManager *manager;
  GtkWidget *main_window;
  GtkWidget *slider;

  gtk_init (&argc, &argv);

  manager = dalston_brightness_manager_new ();
  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  slider = dalston_brightness_slider_new (manager);
  gtk_container_add (GTK_CONTAINER (main_window), slider);
  gtk_widget_show_all (main_window);

  dalston_brightness_manager_start_monitoring (manager);

  gtk_main ();
}
