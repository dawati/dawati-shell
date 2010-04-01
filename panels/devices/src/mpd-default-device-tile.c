
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

#include "mpd-default-device-tile.h"
#include "mpd-shell-defines.h"
#include "config.h"

G_DEFINE_TYPE (MpdDefaultDeviceTile, mpd_default_device_tile, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_DEFAULT_DEVICE_TILE, MpdDefaultDeviceTilePrivate))

enum
{
  REQUEST_HIDE,

  LAST_SIGNAL
};

typedef struct
{
  int dummy;
} MpdDefaultDeviceTilePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_default_device_tile_parent_class)->dispose (object);
}

static void
mpd_default_device_tile_class_init (MpdDefaultDeviceTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdDefaultDeviceTilePrivate));

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
mpd_default_device_tile_init (MpdDefaultDeviceTile *self)
{
  ClutterActor  *hbox;
  ClutterActor  *icon;
  ClutterActor  *label;
  ClutterActor  *separator;
  GError        *error = NULL;

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self), MX_ORIENTATION_VERTICAL);

  hbox = mx_box_layout_new ();
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (hbox),
                             MPD_STORAGE_DEVICE_TILE_SPACING);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), hbox);

  icon = clutter_texture_new_from_file (PKGICONDIR "/device-usb.png",
                                        &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  } else {
    clutter_texture_set_sync_size (CLUTTER_TEXTURE (icon), true);
    clutter_container_add_actor (CLUTTER_CONTAINER (hbox), icon);
    clutter_container_child_set (CLUTTER_CONTAINER (hbox), icon,
                                 "expand", false,
                                 "x-align", MX_ALIGN_START,
                                 "x-fill", false,
                                 "y-align", MX_ALIGN_START,
                                 "y-fill", false,
                                 NULL);
  }

  label = mx_label_new_with_text (_("Plug in a device\n"
                                    "and it will be automatically detected."));
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);
  clutter_container_child_set (CLUTTER_CONTAINER (hbox), label,
                                "expand", true,
                                "x-align", MX_ALIGN_START,
                                "x-fill", true,
                                "y-align", MX_ALIGN_MIDDLE,
                                "y-fill", false,
                                NULL);

  /* Separator */
  separator = mx_icon_new ();
  clutter_actor_set_height (separator, 1.0);
  mx_stylable_set_style_class (MX_STYLABLE (separator), "separator");
  clutter_container_add_actor (CLUTTER_CONTAINER (self), separator);
}

ClutterActor *
mpd_default_device_tile_new (void)
{
  return g_object_new (MPD_TYPE_DEFAULT_DEVICE_TILE, NULL);
}

