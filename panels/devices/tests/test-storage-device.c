
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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <clutter/clutter.h>
#include "mpd-storage-device.h"

static void
_has_media_cb (MpdStorageDevice *storage,
               bool              has_media,
               void             *data)
{
  g_debug ("%s() %d", __FUNCTION__, has_media);
}

int
main (int     argc,
      char  **argv)
{
  MpdStorageDevice  *storage;
  char const        *path;
  bool               query_media;
  int64_t            size;
  int64_t            available_size;

  clutter_init (&argc, &argv);

  if (argc > 1)
  {
    path = argv[1];
    query_media = true;
  } else {
    path = g_get_home_dir ();
    query_media = false;
  }

  storage = mpd_storage_device_new (path);
  g_object_get (storage,
                "size", &size,
                "available-size", &available_size,
                NULL);

  g_debug ("Size: %lld, available: %lld", size, available_size);

  if (query_media)
  {
    g_signal_connect (storage, "has-media",
                      G_CALLBACK (_has_media_cb), NULL);
#if 0
    mpd_storage_device_has_media_async (storage);
#endif
    clutter_main ();
  }

  g_object_unref (storage);
  return EXIT_SUCCESS;
}

