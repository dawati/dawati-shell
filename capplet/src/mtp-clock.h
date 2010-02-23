/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mtp-toolbar-clock.h */
/*
 * Copyright (c) 2009, 2010 Intel Corp.
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

#ifndef _MTP_CLOCK
#define _MTP_CLOCK

#include <mx/mx.h>

G_BEGIN_DECLS

#define MTP_TYPE_CLOCK mtp_clock_get_type()

#define MTP_CLOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MTP_TYPE_CLOCK, MtpClock))

#define MTP_CLOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MTP_TYPE_CLOCK, MtpClockClass))

#define MTP_IS_CLOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MTP_TYPE_CLOCK))

#define MTP_IS_CLOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MTP_TYPE_CLOCK))

#define MTP_CLOCK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MTP_TYPE_CLOCK, MtpClockClass))

typedef struct _MtpClockPrivate MtpClockPrivate;

typedef struct {
  MxWidget parent;

  /*< private >*/
  MtpClockPrivate *priv;
} MtpClock;

typedef struct {
  MxWidgetClass parent_class;
} MtpClockClass;

GType mtp_clock_get_type (void);

ClutterActor* mtp_clock_new (void);

G_END_DECLS

#endif /* _MTP_CLOCK */

