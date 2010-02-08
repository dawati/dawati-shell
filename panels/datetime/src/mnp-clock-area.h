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


#ifndef MNP_CLOCK_AREA_H
#define MNP_CLOCK_AREA_H

#include <glib.h>
#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MNP_TYPE_CLOCK_AREA             (mnp_clock_area_get_type ())
#define MNP_CLOCK_AREA(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNP_TYPE_CLOCK_AREA, MnpClockArea))
#define MNP_CLOCK_AREA_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), MNP_TYPE_CLOCK_AREA, MnpClockAreaClass))
#define	MNP_IS_CLOCK_AREA_TYPE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNP_TYPE_CLOCK_AREA))
#define MNP_IS_CLOCK_AREA_TYPE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), MNP_TYPE_CLOCK_AREA))
#define MNP_CLOCK_AREA_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), MNP_TYPE_CLOCK_AREA, MnpClockAreaClass))

typedef struct _MnpClockArea MnpClockArea;
typedef struct _MnpClockAreaClass MnpClockAreaClass;
typedef struct _MnpClockAreaPriv MnpClockAreaPriv;

struct _MnpClockArea
{
	MxBoxLayout parent;	

	MnpClockAreaPriv *priv;
};

struct _MnpClockAreaClass 
{
	MxBoxLayoutClass parent;
};

GType mnp_detail_type_get_type (void);
MnpClockArea * mnp_clock_area_new (void);
void mnp_clock_area_refresh_time (MnpClockArea *);
time_t mnp_clock_area_get_time (MnpClockArea *area);

G_END_DECLS

#endif
