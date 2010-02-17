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

#ifndef _MNP_ALARM_INSTANCE_H
#define _MNP_ALARM_INSTANCE_H

#include <glib-object.h>
#include "mnp-alarm-utils.h"
#include <time.h>

G_BEGIN_DECLS

#define MNP_TYPE_ALARM_INSTANCE mnp_alarm_instance_get_type()

#define MNP_ALARM_INSTANCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MNP_TYPE_ALARM_INSTANCE, MnpAlarmInstance))

#define MNP_ALARM_INSTANCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MNP_TYPE_ALARM_INSTANCE, MnpAlarmInstanceClass))

#define MNP_IS_ALARM_INSTANCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MNP_TYPE_ALARM_INSTANCE))

#define MNP_IS_ALARM_INSTANCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MNP_TYPE_ALARM_INSTANCE))

#define MNP_ALARM_INSTANCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MNP_TYPE_ALARM_INSTANCE, MnpAlarmInstanceClass))

typedef struct {
  GObject parent;
} MnpAlarmInstance;

typedef struct {
  GObjectClass parent_class;

  void (*alarm_changed) (MnpAlarmInstance *);
} MnpAlarmInstanceClass;

GType mnp_alarm_instance_get_type (void);

MnpAlarmInstance* mnp_alarm_instance_new (MnpAlarmItem *, time_t now);
time_t mnp_alarm_instance_get_time (MnpAlarmInstance *alarm);
void mnp_alarm_instance_remanipulate (MnpAlarmInstance *alarm, time_t now);

G_END_DECLS

#endif /* _MNP_ALARM_INSTANCE_H */
