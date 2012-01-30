/*
 * Copyright © 2012 Intel Corp.
 *
 * Authors: Damien Lespiau <damien.lespiau@intel.com>
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

#include "mpd-disk-tile.h"
#include "mpd-fs-pane.h"
#include "mpd-folder-tile.h"
#include "mpd-shell-defines.h"

G_DEFINE_TYPE (MpdFsPane, mpd_fs_pane, MX_TYPE_BOX_LAYOUT)

#define FS_PANEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_FS_PANEL, MpdFsPanePrivate))

enum
{
  REQUEST_HIDE,

  LAST_SIGNAL
};

struct _MpdFsPanePrivate
{
  gint dummy;
};

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static void
_tile_request_hide_cb (ClutterActor *tile,
                       MpdFsPane    *self)
{
  g_signal_emit_by_name (self, "request-hide");
}

static void
mpd_fs_pane_class_init (MpdFsPaneClass *klass)
{
  g_type_class_add_private (klass, sizeof (MpdFsPanePrivate));

  /* Signals */

  _signals[REQUEST_HIDE] = g_signal_new ("request-hide",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);
}

static void
mpd_fs_pane_init (MpdFsPane *self)
{
  ClutterActor  *tile;

  self->priv = FS_PANEL_PRIVATE (self);

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self), MX_ORIENTATION_VERTICAL);
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), 21);
  clutter_actor_set_width (CLUTTER_ACTOR (self), MPD_PANE_WIDTH);

  tile = mpd_disk_tile_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (self), tile);

  tile = mpd_folder_tile_new ();
  clutter_actor_set_width (tile,  MPD_FOLDER_TILE_WIDTH);
  g_signal_connect (tile, "request-hide",
                    G_CALLBACK (_tile_request_hide_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), tile);
  clutter_container_child_set (CLUTTER_CONTAINER (self), tile,
                               "expand", TRUE,
                               "x-fill", TRUE,
                               "y-fill", TRUE,
                               NULL);
}

ClutterActor *
mpd_fs_pane_new (void)
{
  return g_object_new (MPD_TYPE_FS_PANEL, NULL);
}
