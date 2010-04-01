
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
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <mx/mx.h>
#include "mpd-shell-defines.h"
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

static void
_find_first_cb (ClutterActor   *actor,
                void          **child)
{
  if (NULL == *child)
    *child = actor;
}

static void
_mount_added_cb (GVolumeMonitor  *monitor,
                 GMount          *mount,
                 ClutterActor    *box)
{
  GFile         *file;
  char          *name;
  char          *path;
  GIcon         *icon;
  GtkIconInfo   *icon_info;
  char const    *icon_file;
  ClutterActor  *tile;
  ClutterActor  *label = NULL;

  clutter_container_foreach (CLUTTER_CONTAINER (box),
                             (ClutterCallback) _find_first_cb,
                             &label);
  if (MX_IS_LABEL (label))
  {
    clutter_actor_destroy (label);
  } else {
    g_debug ("Not a label");
  }

  /* Mount point */
  file = g_mount_get_root (mount);
  path = g_file_get_path (file);
  name = g_mount_get_name (mount);

  /* Icon */
  icon = g_mount_get_icon (mount);
  icon_info = gtk_icon_theme_lookup_by_gicon (gtk_icon_theme_get_default (),
                                              icon,
                                              MPD_STORAGE_DEVICE_TILE_ICON_SIZE,
                                              GTK_ICON_LOOKUP_NO_SVG);
  icon_file = gtk_icon_info_get_filename (icon_info);
  g_debug ("%s() %s", __FUNCTION__, icon_file);

  tile = mpd_storage_device_tile_new (name, path, NULL, icon_file);
  g_signal_connect (tile, "eject",
                    G_CALLBACK (_tile_unmount_cb), g_object_ref (mount));
  mx_box_layout_add_actor (MX_BOX_LAYOUT (box), tile, 0);
  clutter_actor_set_width (tile, 480.0);

  gtk_icon_info_free (icon_info);
  g_object_unref (icon);
  g_free (name);
  g_free (path);
  g_object_unref (file);
}

static void
_mount_changed_cb (GVolumeMonitor *monitor,
                   GMount         *mount,
                   ClutterActor   *box)
{
  g_debug ("%s()", __FUNCTION__);
}

static void
_mount_removed_cb (GVolumeMonitor  *monitor,
                   GMount          *mount,
                   ClutterActor    *box)
{
  ClutterActor *actor = NULL;

  clutter_container_foreach (CLUTTER_CONTAINER (box),
                             (ClutterCallback) _find_first_cb,
                             &actor);
  if (MPD_IS_STORAGE_DEVICE_TILE (actor))
  {
    clutter_actor_destroy (actor);
  } else {
    g_debug ("Not a tile");
  }

  actor = mx_label_new_with_text ("Plug in USB storage device ...");
  mx_box_layout_add_actor (MX_BOX_LAYOUT (box), actor, 0);
}

int
main (int     argc,
      char  **argv)
{
  ClutterActor    *stage;
  ClutterActor    *box;
  ClutterActor    *label;
  GVolumeMonitor  *monitor;

  clutter_init (&argc, &argv);
  /* Just for icon theme, no widgets. */
  gtk_init (&argc, &argv);

  stage = clutter_stage_get_default ();
  box = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (box), MX_ORIENTATION_VERTICAL);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), box);

  monitor = g_volume_monitor_get ();
  g_signal_connect (monitor, "mount-added",
                    G_CALLBACK (_mount_added_cb), box);
  g_signal_connect (monitor, "mount-changed",
                    G_CALLBACK (_mount_changed_cb), box);
  g_signal_connect (monitor, "mount-removed",
                    G_CALLBACK (_mount_removed_cb), box);

  label = mx_label_new_with_text ("Plug in USB storage device ...");
  clutter_container_add_actor (CLUTTER_CONTAINER (box), label);

  clutter_actor_show_all (stage);
  clutter_main ();

  g_object_unref (monitor);

  return EXIT_SUCCESS;
}

