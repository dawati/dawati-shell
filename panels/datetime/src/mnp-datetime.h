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

#ifndef _MNP_DATETIME
#define _MNP_DATETIME

#include <glib-object.h>
#include <mx/mx.h>
#include <meego-panel/mpl-panel-clutter.h>
#include <meego-panel/mpl-panel-common.h>

G_BEGIN_DECLS

#define MNP_TYPE_DATETIME mnp_datetime_get_type()

#define MNP_DATETIME(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNP_TYPE_DATETIME, MnpDatetime))

#define MNP_DATETIME_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNP_TYPE_DATETIME, MnpDatetimeClass))

#define MNP_IS_DATETIME(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNP_TYPE_DATETIME))

#define MNP_IS_DATETIME_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNP_TYPE_DATETIME))

#define MNP_DATETIME_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNP_TYPE_DATETIME, MnpDatetimeClass))

typedef struct _MnpDatetimePrivate MnpDatetimePrivate;

typedef struct {
  MxBoxLayout parent;

  MnpDatetimePrivate *priv;
} MnpDatetime;

typedef struct {
  MxBoxLayoutClass parent_class;
} MnpDatetimeClass;

GType mnp_datetime_get_type (void);

ClutterActor *mnp_datetime_new (void);
void mnp_datetime_set_panel_client (MnpDatetime *datetime,
                                        MplPanelClient *panel_client);
void
mnp_date_time_set_date_label (MnpDatetime *datetime, ClutterActor *label);

G_END_DECLS

#endif /* _MNP_DATETIME */
