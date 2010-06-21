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

#ifndef _MNP_WORLD_CLOCK
#define _MNP_WORLD_CLOCK

#include <glib-object.h>
#include <mx/mx.h>
#include <meego-panel/mpl-panel-clutter.h>
#include <meego-panel/mpl-panel-common.h>

G_BEGIN_DECLS

#define MNP_TYPE_WORLD_CLOCK mnp_world_clock_get_type()

#define MNP_WORLD_CLOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNP_TYPE_WORLD_CLOCK, MnpWorldClock))

#define MNP_WORLD_CLOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNP_TYPE_WORLD_CLOCK, MnpWorldClockClass))

#define MNP_IS_WORLD_CLOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNP_TYPE_WORLD_CLOCK))

#define MNP_IS_WORLD_CLOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNP_TYPE_WORLD_CLOCK))

#define MNP_WORLD_CLOCK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNP_TYPE_WORLD_CLOCK, MnpWorldClockClass))

typedef struct {
  MxBoxLayout parent;
} MnpWorldClock;

typedef struct {
  MxBoxLayoutClass parent_class;
  void (*time_changed) (MnpWorldClock *);
} MnpWorldClockClass;

GType mnp_world_clock_get_type (void);

ClutterActor *mnp_world_clock_new (void);
void mnp_world_clock_set_client (MnpWorldClock *clock, MplPanelClient *client);

G_END_DECLS

#endif /* _MNP_WORLD_CLOCK */
