
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

#include "mpd-battery-tile.h"
#include "mpd-computer-tile.h"
#include "mpd-disk-tile.h"
#include "mpd-volume-tile.h"
#include "config.h"

G_DEFINE_TYPE (MpdComputerTile, mpd_computer_tile, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_COMPUTER_TILE, MpdComputerTilePrivate))

enum
{
  REQUEST_HIDE,

  LAST_SIGNAL
};

typedef struct
{
  int dummy;
} MpdComputerTilePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static void
_settings_clicked_cb (MxButton        *button,
                      MpdComputerTile *self)
{
  GError *error = NULL;

  g_spawn_command_line_async ("gnome-control-center", &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  g_signal_emit_by_name (self, "request-hide");
}

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_computer_tile_parent_class)->dispose (object);
}

static void
mpd_computer_tile_class_init (MpdComputerTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdComputerTilePrivate));

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
mpd_computer_tile_init (MpdComputerTile *self)
{
  ClutterActor *button;
  ClutterActor *tile;

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self), MX_ORIENTATION_VERTICAL);
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), 12);

  tile = mpd_battery_tile_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (self), tile);

  tile = mpd_disk_tile_new ();
  mx_stylable_set_style_class (MX_STYLABLE (tile), "frame");
  clutter_container_add_actor (CLUTTER_CONTAINER (self), tile);

  tile = mpd_volume_tile_new ();
  mx_stylable_set_style_class (MX_STYLABLE (tile), "frame");
  clutter_container_add_actor (CLUTTER_CONTAINER (self), tile);

  button = mx_button_new_with_label (_("All settings"));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (_settings_clicked_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), button);
  clutter_container_child_set (CLUTTER_CONTAINER (self), button,
                               "expand", true,
                               "x-align", MX_ALIGN_START,
                               "x-fill", true,
                               "y-align", MX_ALIGN_END,
                               "y-fill", false,
                               NULL);
}

ClutterActor *
mpd_computer_tile_new (void)
{
  return g_object_new (MPD_TYPE_COMPUTER_TILE, NULL);
}


