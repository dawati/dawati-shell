
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
#include <stdint.h>
#include <glib/gi18n.h>
#include "mpd-disk-device.h"
#include "mpd-disk-tile.h"

G_DEFINE_TYPE (MpdDiskTile, mpd_disk_tile, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_DISK_TILE, MpdDiskTilePrivate))

typedef struct
{
  /* Managed by clutter */
  ClutterActor  *label;
  ClutterActor  *meter;

  /* Data */
  MpdDiskDevice *disk;
} MpdDiskTilePrivate;

static void
update (MpdDiskTile *self)
{
  MpdDiskTilePrivate *priv = GET_PRIVATE (self);
  char      *text;
  char      *size_text;
  uint64_t   size;
  uint64_t   available_size;
  guint      percentage;

  g_object_get (priv->disk,
                "size", &size,
                "available-size", &available_size,
                NULL);

  percentage = 100 - (double) available_size / size * 100;

  size_text = g_format_size_for_display (size);
  text = g_strdup_printf (_("You are using %d%% of %s"), percentage, size_text);
  mx_label_set_text (MX_LABEL (priv->label), text);
  g_free (text);
  g_free (size_text);

  mx_progress_bar_set_progress (MX_PROGRESS_BAR (priv->meter), percentage / 100.);
}

static void
_disk_size_notify_cb (MpdDiskDevice *battery,
                      GParamSpec    *pspec,
                      MpdDiskTile   *self)
{
  update (self);
}

static void
_dispose (GObject *object)
{
  MpdDiskTilePrivate *priv = GET_PRIVATE (object);

  if (priv->disk)
  {
    g_object_unref (priv->disk);
    priv->disk = NULL;
  }

  G_OBJECT_CLASS (mpd_disk_tile_parent_class)->dispose (object);
}

static void
mpd_disk_tile_class_init (MpdDiskTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdDiskTilePrivate));

  object_class->dispose = _dispose;
}

static void
mpd_disk_tile_init (MpdDiskTile *self)
{
  MpdDiskTilePrivate *priv = GET_PRIVATE (self);

  mx_box_layout_set_vertical (MX_BOX_LAYOUT (self), true);
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), 6);

  priv->disk = mpd_disk_device_new ();
  g_signal_connect (priv->disk, "notify::size",
                    G_CALLBACK (_disk_size_notify_cb), self);
  g_signal_connect (priv->disk, "notify::available-size",
                    G_CALLBACK (_disk_size_notify_cb), self);

  priv->label = mx_label_new ("");
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->label);

  priv->meter = mx_progress_bar_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->meter);
  clutter_container_child_set (CLUTTER_CONTAINER (self), priv->meter,
                               "expand", true,
                               "x-fill", true,
                               NULL);

  update (self);
}

ClutterActor *
mpd_disk_tile_new (void)
{
  return g_object_new (MPD_TYPE_DISK_TILE, NULL);
}

