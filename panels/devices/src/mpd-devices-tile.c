
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

#include "mpd-devices-tile.h"

G_DEFINE_TYPE (MpdDevicesTile, mpd_devices_tile, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_DEVICES_TILE, MpdDevicesTilePrivate))

typedef struct
{
    int dummy;
} MpdDevicesTilePrivate;

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_devices_tile_parent_class)->dispose (object);
}

static void
mpd_devices_tile_class_init (MpdDevicesTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdDevicesTilePrivate));

  object_class->dispose = _dispose;
}

static void
mpd_devices_tile_init (MpdDevicesTile *self)
{
}

MpdDevicesTile *
mpd_devices_tile_new (void)
{
  return g_object_new (MPD_TYPE_DEVICES_TILE, NULL);
}

