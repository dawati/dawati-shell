/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Srinivasa Ragavan <srini@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MNP_CLOCK_TILE_H
#define MNP_CLOCK_TILE_H

#include <glib.h>
#include <glib-object.h>

#include <mx/mx.h>
#include "mnp-utils.h"

G_BEGIN_DECLS

#define MNP_TYPE_CLOCK_TILE             (mnp_clock_tile_get_type ())
#define MNP_CLOCK_TILE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNP_TYPE_CLOCK_TILE, MnpClockTile))
#define MNP_CLOCK_TILE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), MNP_TYPE_CLOCK_TILE, MnpClockTileClass))
#define MNP_IS_CLOCK_TILE_TYPE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNP_TYPE_CLOCK_TILE))
#define MNP_IS_CLOCK_TILE_TYPE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), MNP_TYPE_CLOCK_TILE))
#define MNP_CLOCK_TILE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), MNP_TYPE_CLOCK_TILE, MnpClockTileClass))

typedef struct _MnpClockTile MnpClockTile;
typedef struct _MnpClockTileClass MnpClockTileClass;
typedef struct _MnpClockTilePriv MnpClockTilePriv;

struct _MnpClockTile
{
	MxBoxLayout parent;	

	MnpClockTilePriv *priv;
};

struct _MnpClockTileClass 
{
	MxBoxLayoutClass parent;
	void (* drag_y_pos) (MnpClockTile *tile, gint pos);
	
};

typedef void (*TileRemoveFunc) (MnpClockTile *, gpointer data);

GType mnp_clock_tile_get_type (void);
MnpClockTile * mnp_clock_tile_new (MnpZoneLocation *, time_t time_now);
void mnp_clock_tile_set_remove_cb (MnpClockTile *tile, TileRemoveFunc func, gpointer data);
void mnp_clock_tile_refresh (MnpClockTile *tile, time_t now, gboolean tfh);
MnpZoneLocation * mnp_clock_tile_get_location (MnpClockTile *tile);

G_END_DECLS

#endif
