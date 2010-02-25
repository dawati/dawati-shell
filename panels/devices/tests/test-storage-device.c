
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Robert Staudinger <robert.staudinger@intel.com>
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
 */

#include <stdint.h>
#include <stdlib.h>
#include <clutter/clutter.h>
#include "mpd-storage-device.h"

int
main (int     argc,
      char  **argv)
{
  MpdStorageDevice  *storage;
  int64_t            size;
  int64_t            available_size;

  clutter_init (&argc, &argv);

  storage = mpd_storage_device_new_for_path (g_get_home_dir ());
  g_object_get (storage,
                "size", &size,
                "available-size", &available_size,
                NULL);

  g_debug ("Size: %lld, available: %lld", size, available_size);

  g_object_unref (storage);
  return EXIT_SUCCESS;
}

