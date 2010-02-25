
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
#include <gio/gio.h>
#include <mx/mx.h>
#include "mpd-storage-device-tile.h"

static void
_eject_ready_cb (GDrive       *drive,
                 GAsyncResult *result,
                 void         *data)
{
  GError *error = NULL;

  g_debug ("%s()", __FUNCTION__);

  g_drive_eject_with_operation_finish (drive, result, &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
}

/*
static void
_unmount_ready_cb (GMount       *mount,
                   GAsyncResult *result,
                   void         *data)
{
  GError *error = NULL;

  g_debug ("%s()", __FUNCTION__);

  g_mount_unmount_with_operation_finish (mount, result, &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
}
*/

static void
_tile_unmount_cb (MpdStorageDeviceTile  *tile,
                  GMount                *mount)
{
  GDrive *drive;

  g_debug ("%s()", __FUNCTION__);

  drive = g_mount_get_drive (mount);
  g_drive_eject_with_operation (drive,
                                G_MOUNT_UNMOUNT_NONE,
                                NULL,
                                NULL,
                                (GAsyncReadyCallback) _eject_ready_cb,
                                tile);

/*
  g_mount_unmount_with_operation (mount,
                                  G_MOUNT_UNMOUNT_NONE,
                                  NULL,
                                  NULL,
                                  (GAsyncReadyCallback) _unmount_ready_cb,
                                  tile);
*/
  g_object_unref (drive);
  g_object_unref (mount);
}

static int
_find_uuid_cb (GduDevice  *device,
               char const *uuid)
{
  char const *device_uuid;

  device_uuid = gdu_device_id_get_uuid (device);

  g_debug ("%s() %s", __FUNCTION__, device_uuid);

  return g_strcmp0 (uuid, device_uuid);
}

GduDevice *
device_from_mount (GMount *mount)
{
  GduPool   *pool;
  GList     *devices;
  GList     *iter;
  GduDevice *device;
  GVolume   *volume;
  char      *identifier;
  char      *name;
  char      *uuid;

  volume = g_mount_get_volume (mount);

  identifier = g_volume_get_identifier (volume, G_VOLUME_IDENTIFIER_KIND_UUID);
  name = g_volume_get_name (volume);
  uuid = g_volume_get_uuid (volume);

  g_debug ("%s() %s %s %s ", __FUNCTION__,
           identifier,
           name,
           uuid);

  pool = gdu_pool_new ();
  devices = gdu_pool_get_devices (pool);
  iter = g_list_find_custom (devices, identifier, (GCompareFunc) _find_uuid_cb);
  if (iter)
  {
    device = g_object_ref (iter->data);
  } else {
    g_warning ("%s : Could not find device '%s' in g-d-u.", G_STRLOC, identifier);
    device = NULL;
  }

  g_list_foreach (devices, (GFunc) g_object_unref, NULL);
  g_list_free (devices);
  g_object_unref (pool);

  g_free (identifier);
  g_free (name);
  g_free (uuid);

  return device;
}

static void
_remove_cb (ClutterActor      *actor,
            ClutterContainer  *container)
{
  clutter_container_remove_actor (container, actor);
}

static void
_mount_added_cb (GVolumeMonitor  *monitor,
                 GMount          *mount,
                 ClutterActor    *stage)
{
  GduDevice *device;

  clutter_container_foreach (CLUTTER_CONTAINER (stage),
                             (ClutterCallback) _remove_cb,
                             stage);

  device = device_from_mount (mount);
  g_return_if_fail (device);

  g_debug ("%s() mounted: '%d', name: '%s', vendor: '%s', model: '%s'\n"
           "uid: '%s'\n"
           "ejectable: '%d'",
           __FUNCTION__, 
           gdu_device_is_mounted (device),
           gdu_device_get_presentation_name (device),
           gdu_device_drive_get_vendor (device),
           gdu_device_drive_get_model (device),
           gdu_device_id_get_uuid (device),
           gdu_device_drive_get_is_media_ejectable (device));

  if (gdu_device_is_mounted (device))
  {
    ClutterActor *tile;

    tile = mpd_storage_device_tile_new (gdu_device_get_mount_path (device));
    g_signal_connect (tile, "eject",
                      G_CALLBACK (_tile_unmount_cb), g_object_ref (mount));
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), tile);
    clutter_actor_set_width (tile, 480.0);
  }
}

static void
_mount_changed_cb (GVolumeMonitor *monitor,
                   GMount         *mount,
                   ClutterActor   *stage)
{
  g_debug ("%s()", __FUNCTION__);
}

static void
_mount_removed_cb (GVolumeMonitor  *monitor,
                   GMount          *mount,
                   ClutterActor    *stage)
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
  ClutterActor    *stage;
  ClutterActor    *label;
  GVolumeMonitor  *monitor;

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();

  monitor = g_volume_monitor_get ();
  g_signal_connect (monitor, "mount-added",
                    G_CALLBACK (_mount_added_cb), stage);
  g_signal_connect (monitor, "mount-changed",
                    G_CALLBACK (_mount_changed_cb), stage);
  g_signal_connect (monitor, "mount-removed",
                    G_CALLBACK (_mount_removed_cb), stage);

  label = mx_label_new ("Plug in USB storage device ...");
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), label);

  clutter_actor_show_all (stage);
  clutter_main ();

  g_object_unref (monitor);

  return EXIT_SUCCESS;
}

