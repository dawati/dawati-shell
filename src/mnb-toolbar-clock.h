/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-toolbar-clock.h */
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _MNB_TOOLBAR_CLOCK
#define _MNB_TOOLBAR_CLOCK

#include <glib-object.h>

#include "mnb-toolbar-button.h"

G_BEGIN_DECLS

#define MNB_TYPE_TOOLBAR_CLOCK mnb_toolbar_clock_get_type()

#define MNB_TOOLBAR_CLOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_TOOLBAR_CLOCK, MnbToolbarClock))

#define MNB_TOOLBAR_CLOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_TOOLBAR_CLOCK, MnbToolbarClockClass))

#define MNB_IS_TOOLBAR_CLOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_TOOLBAR_CLOCK))

#define MNB_IS_TOOLBAR_CLOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_TOOLBAR_CLOCK))

#define MNB_TOOLBAR_CLOCK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_TOOLBAR_CLOCK, MnbToolbarClockClass))

typedef struct _MnbToolbarClockPrivate MnbToolbarClockPrivate;

typedef struct {
  MnbToolbarButton parent;

  /*< private >*/
  MnbToolbarClockPrivate *priv;
} MnbToolbarClock;

typedef struct {
  MnbToolbarButtonClass parent_class;
} MnbToolbarClockClass;

GType mnb_toolbar_clock_get_type (void);

ClutterActor* mnb_toolbar_clock_new (void);

G_END_DECLS

#endif /* _MNB_TOOLBAR_CLOCK */

