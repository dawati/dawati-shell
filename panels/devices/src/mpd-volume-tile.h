
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

#ifndef MPD_VOLUME_TILE_H
#define MPD_VOLUME_TILE_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MPD_TYPE_VOLUME_TILE mpd_volume_tile_get_type()

#define MPD_VOLUME_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_VOLUME_TILE, MpdVolumeTile))

#define MPD_VOLUME_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_VOLUME_TILE, MpdVolumeTileClass))

#define MPD_IS_VOLUME_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_VOLUME_TILE))

#define MPD_IS_VOLUME_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_VOLUME_TILE))

#define MPD_VOLUME_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_VOLUME_TILE, MpdVolumeTileClass))

typedef struct
{
  MxBoxLayout parent;
} MpdVolumeTile;

typedef struct
{
  MxBoxLayoutClass parent;
} MpdVolumeTileClass;

GType
mpd_volume_tile_get_type (void);

ClutterActor *
mpd_volume_tile_new (void);

G_END_DECLS

#endif /* MPD_VOLUME_TILE_H */

