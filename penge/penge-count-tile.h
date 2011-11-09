/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
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

#ifndef PENGE_COUNT_TILE_H
#define PENGE_COUNT_TILE_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define PENGE_TYPE_COUNT_TILE penge_count_tile_get_type()

#define PENGE_COUNT_TILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_COUNT_TILE, PengeCountTile))

#define PENGE_COUNT_TILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_COUNT_TILE, PengeCountTileClass))

#define MAI_IS_VOLUME_CONTROL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_COUNT_TILE))

#define MAI_IS_VOLUME_CONTROL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_COUNT_TILE))

#define PENGE_COUNT_TILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_COUNT_TILE, PengeCountTileClass))

typedef struct _PengeCountTilePrivate PengeCountTilePrivate;

typedef struct
{
  MxButton parent;
  PengeCountTilePrivate *priv;
} PengeCountTile;

typedef struct
{
  MxScrollBarClass parent_class;
} PengeCountTileClass;

GType         penge_count_tile_get_type (void);
MxWidget     *penge_count_tile_new (void);

G_END_DECLS

#endif /* PENGE_COUNT_TILE_H */

