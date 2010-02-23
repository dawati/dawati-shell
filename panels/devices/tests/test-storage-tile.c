
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

#include <stdlib.h>
#include <clutter/clutter.h>
#include <gdu/gdu.h>
#include <mx/mx.h>
#include "mpd-storage-tile.h"

static void
_remove_cb (ClutterActor      *actor,
            ClutterContainer  *container)
{
  clutter_container_remove_actor (container, actor);
}

static void
_device_added_cb (GduPool       *pool,
                  GduDevice     *device,
                  ClutterActor  *stage)
{
  g_debug ("%s()", __FUNCTION__);
}

static void
_device_changed_cb (GduPool       *pool,
                    GduDevice     *device,
                    ClutterActor  *stage)
{
  clutter_container_foreach (CLUTTER_CONTAINER (stage),
                             (ClutterCallback) _remove_cb,
                             stage);

  g_debug ("%s() mounted: '%d', name: '%s', vendor: '%s', model: '%s'",
           __FUNCTION__, 
           gdu_device_is_mounted (device),
           gdu_device_get_presentation_name (device),
           gdu_device_drive_get_vendor (device),
           gdu_device_drive_get_model (device));

  if (gdu_device_is_mounted (device))
  {
    ClutterActor *tile;
    char *title;
    char const *name = gdu_device_get_presentation_name (device);
    char const *vendor = gdu_device_drive_get_vendor (device);
    char const *model = gdu_device_drive_get_model (device);

    if (name && *name)
      title = g_strdup_printf ("%s - %s %s", name, vendor, model);
    else
      title = g_strdup_printf ("%s %s", vendor, model);
    
    tile = mpd_storage_tile_new (gdu_device_get_mount_path (device), title);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), tile);
    clutter_actor_set_width (tile, 480.0);

    g_free (title);  
  }
}

static void
_device_removed_cb (GduPool       *pool,
                    GduDevice     *device,
                    ClutterActor  *stage)
{
  ClutterActor  *label;

  clutter_container_foreach (CLUTTER_CONTAINER (stage),
                             (ClutterCallback) _remove_cb,
                             stage);

  label = mx_label_new ("Plug in USB storage device ...");
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), label);
}

int
main (int     argc,
      char  **argv)
{
  ClutterActor  *stage;
  ClutterActor  *label;
  GduPool       *pool;

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();

  pool = gdu_pool_new ();
  g_signal_connect (pool, "device-added",
                    G_CALLBACK (_device_added_cb), stage);
  g_signal_connect (pool, "device-changed",
                    G_CALLBACK (_device_changed_cb), stage);
  g_signal_connect (pool, "device-removed",
                    G_CALLBACK (_device_removed_cb), stage);

  label = mx_label_new ("Plug in USB storage device ...");
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), label);

  clutter_actor_show_all (stage);
  clutter_main ();

  g_object_unref (pool);

  return EXIT_SUCCESS;
}

