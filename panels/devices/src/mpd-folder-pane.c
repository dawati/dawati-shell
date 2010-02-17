
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
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

#include <glib/gi18n.h>

#include "mpd-devices-tile.h"
#include "mpd-folder-pane.h"
#include "mpd-folder-tile.h"
#include "mpd-shell-defines.h"
#include "config.h"

G_DEFINE_TYPE (MpdFolderPane, mpd_folder_pane, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_FOLDER_PANE, MpdFolderPanePrivate))

enum
{
  REQUEST_HIDE,

  LAST_SIGNAL
};

typedef struct
{
  int dummy;
} MpdFolderPanePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static void
_folder_tile_request_hide_cb (MpdFolderTile  *tile,
                              MpdFolderPane  *self)
{
  g_signal_emit_by_name (self, "request-hide");
}

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_folder_pane_parent_class)->dispose (object);
}

static void
mpd_folder_pane_class_init (MpdFolderPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdFolderPanePrivate));

  object_class->dispose = _dispose;

  /* Signals */

  _signals[REQUEST_HIDE] = g_signal_new ("request-hide",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);
}

static void
mpd_folder_pane_init (MpdFolderPane *self)
{
  ClutterActor *label;
  ClutterActor *tile;

  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), MPD_SHELL_SPACING);
  mx_box_layout_set_vertical (MX_BOX_LAYOUT (self), true);

  label = mx_label_new (_("Your computer"));
  mx_stylable_set_style_class (MX_STYLABLE (label), "panel-title");
  clutter_container_add_actor (CLUTTER_CONTAINER (self), label);

  tile = mpd_folder_tile_new ();
  g_signal_connect (tile, "request-hide",
                    G_CALLBACK (_folder_tile_request_hide_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), tile);

  tile = mpd_devices_tile_new ();
  g_signal_connect (tile, "request-hide",
                    G_CALLBACK (_folder_tile_request_hide_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), tile);
}

ClutterActor *
mpd_folder_pane_new (void)
{
  return g_object_new (MPD_TYPE_FOLDER_PANE, NULL);
}

