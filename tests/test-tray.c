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


int
main (int argc, char *argv[])
{
  GtkWidget     *config;
  GtkStatusIcon *icon;

  gtk_init (&argc, &argv);

  config = gtk_plug_new (0);

  gtk_container_add (GTK_CONTAINER (config),
                     gtk_image_new_from_stock (GTK_STOCK_QUIT,
                                               GTK_ICON_SIZE_LARGE_TOOLBAR));

  icon = gtk_status_icon_new_from_stock (GTK_STOCK_INFO);

  mnbk_system_tray_init (icon, GTK_PLUG (config));

  gtk_main ();

  return 0;
}
