
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

#include "mpd-computer-pane.h"
#include "mpd-computer-tile.h"
#include "mpd-folder-tile.h"
#include "mpd-shell-defines.h"

G_DEFINE_TYPE (MpdComputerPane, mpd_computer_pane, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_COMPUTER_PANE, MpdComputerPanePrivate))

enum
{
  REQUEST_HIDE,

  LAST_SIGNAL
};

typedef struct
{
    int dummy;
} MpdComputerPanePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static void
_tile_request_hide_cb (ClutterActor     *tile,
                       MpdComputerPane  *self)
{
  g_debug ("%s()", __FUNCTION__);
}

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
  G_OBJECT_CLASS (mpd_computer_pane_parent_class)->dispose (object);
}

static void
mpd_computer_pane_class_init (MpdComputerPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdComputerPanePrivate));

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
mpd_computer_pane_init (MpdComputerPane *self)
{
  ClutterActor  *label;
  ClutterActor  *button;
  ClutterActor  *hbox;
  ClutterActor  *tile;

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self), MX_ORIENTATION_VERTICAL);
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), MPD_PANE_HEADER_SPACING);
  clutter_actor_set_opacity (CLUTTER_ACTOR (self), 0xff * 0.8); /* 80% */
  clutter_actor_set_width (CLUTTER_ACTOR (self), MPD_COMPUTER_PANE_WIDTH);

  /* Title */
  hbox = mx_box_layout_new ();
  clutter_actor_set_name (hbox, "computer-pane-header");
  clutter_container_add_actor (CLUTTER_CONTAINER (self), hbox);
  clutter_container_child_set (CLUTTER_CONTAINER (self), hbox,
                               "expand", false,
                               "x-fill", true,
                               "y-fill", false,
                               NULL);

  label = mx_label_new_with_text (_("Your computer"));
  clutter_actor_set_name (label, "computer-pane-label");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), label,
                               "expand", true,
                               "x-fill", true,
                               "y-fill", false,
                               NULL);

  button = mx_button_new_with_label (_("All settings"));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (_settings_clicked_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), button);

  /* Content */
  hbox = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (hbox), MX_ORIENTATION_HORIZONTAL);
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (hbox), MPD_COMPUTER_PANE_SPACING);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), hbox);

  tile = mpd_computer_tile_new ();
  clutter_actor_set_width (tile,  MPD_COMPUTER_TILE_WIDTH);
  g_signal_connect (tile, "request-hide",
                    G_CALLBACK (_tile_request_hide_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), tile);

  tile = mpd_folder_tile_new ();
  clutter_actor_set_width (tile,  MPD_FOLDER_TILE_WIDTH);
  g_signal_connect (tile, "request-hide",
                    G_CALLBACK (_tile_request_hide_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), tile);
}

ClutterActor *
mpd_computer_pane_new (void)
{
  return g_object_new (MPD_TYPE_COMPUTER_PANE, NULL);
}


