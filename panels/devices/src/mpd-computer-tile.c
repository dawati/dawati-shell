
/*
 * Copyright © 2010, 2012 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
 *          Damien Lespiau <damien.lespiau@intel.com>
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
#include "mpd-brightness-tile.h"
#include "mpd-computer-tile.h"
#include "mpd-display-device.h"
#include "mpd-shell-defines.h"
#include "mpd-volume-tile.h"
#include "config.h"

G_DEFINE_TYPE (MpdComputerTile, mpd_computer_tile, MX_TYPE_TABLE)

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
  ClutterActor      *tile, *label, *toggle, *frame;
  MpdDisplayDevice  *display;
  bool               show_brightness_tile;

  /* Wifi */
  label = mx_label_new_with_text (_("WiFi"));
  mx_table_add_actor_with_properties (MX_TABLE (self), label,
                                      0, 0,
                                      "x-expand", FALSE,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "y-fill", FALSE,
                                      NULL);

  frame = mx_frame_new ();
  toggle = mx_toggle_new ();
  mx_widget_set_disabled (MX_WIDGET (toggle), TRUE);

  mx_bin_set_child (MX_BIN (frame), toggle);
  mx_table_add_actor_with_properties (MX_TABLE (self), frame,
                                      0, 1,
                                      "x-expand", FALSE,
                                      "x-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      NULL);

  /* Bluetooth */
  label = mx_label_new_with_text (_("Bluetooth"));
  mx_table_add_actor_with_properties (MX_TABLE (self), label,
                                      1, 0,
                                      "x-expand", FALSE,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "y-fill", FALSE,
                                      NULL);

  frame = mx_frame_new ();
  toggle = mx_toggle_new ();
  mx_widget_set_disabled (MX_WIDGET (toggle), TRUE);

  mx_bin_set_child (MX_BIN (frame), toggle);
  mx_table_add_actor_with_properties (MX_TABLE (self), frame,
                                      1, 1,
                                      "x-expand", FALSE,
                                      "x-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      NULL);

  /* Flight mode */
  label = mx_label_new_with_text (_("Flight Mode"));
  mx_table_add_actor_with_properties (MX_TABLE (self), label,
                                      2, 0,
                                      "x-expand", FALSE,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "y-fill", FALSE,
                                      NULL);

  frame = mx_frame_new ();
  toggle = mx_toggle_new ();
  mx_widget_set_disabled (MX_WIDGET (toggle), TRUE);

  mx_bin_set_child (MX_BIN (frame), toggle);
  mx_table_add_actor_with_properties (MX_TABLE (self), frame,
                                      2, 1,
                                      "x-expand", FALSE,
                                      "x-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      NULL);

  /* Volume */
  /* Note to translators, volume here is sound volume */
  label = mx_label_new_with_text (_("Volume"));
  mx_table_add_actor_with_properties (MX_TABLE (self), label,
                                      3, 0,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "y-fill", FALSE,
                                      NULL);

  tile = mpd_volume_tile_new ();
  mx_table_add_actor_with_properties (MX_TABLE (self), tile,
                                      3, 1,
                                      "y-expand", FALSE,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      "y-fill", FALSE,
                                      "x-expand", TRUE,
                                      NULL);

  /* Brightness */
  display = mpd_display_device_new ();
  show_brightness_tile = mpd_display_device_is_enabled (display);
  if (show_brightness_tile)
    {
      label = mx_label_new_with_text (_("Brightness"));
      mx_table_add_actor_with_properties (MX_TABLE (self), label,
                                          4, 0,
                                          "y-align", MX_ALIGN_MIDDLE,
                                          "y-fill", FALSE,
                                          NULL);

      tile = mpd_brightness_tile_new ();
      mx_table_add_actor_with_properties (MX_TABLE (self), tile,
                                          4, 1,
                                          "x-expand", TRUE,
                                          NULL);
    }

#if 0
  tile = mpd_battery_tile_new ();
  mx_table_add_actor (MX_TABLE (self), tile, 2, 0);
#endif

  /* FIXME: Makes crash when unref'd.
   * GpmBrightnessXRandR doesn't remove filter from root window in ::finalize()
   * but doesn't seem to be it.
   * g_object_unref (display); */
}

ClutterActor *
mpd_computer_tile_new (void)
{
  return g_object_new (MPD_TYPE_COMPUTER_TILE, NULL);
}


