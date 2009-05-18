/*
 * Dalston - power and volume applets for Moblin
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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
 *
 */


#ifndef _DALSTON_BATTERY_MONITOR
#define _DALSTON_BATTERY_MONITOR

#include <glib-object.h>

G_BEGIN_DECLS

#define DALSTON_TYPE_BATTERY_MONITOR dalston_battery_monitor_get_type()

#define DALSTON_BATTERY_MONITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DALSTON_TYPE_BATTERY_MONITOR, DalstonBatteryMonitor))

#define DALSTON_BATTERY_MONITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DALSTON_TYPE_BATTERY_MONITOR, DalstonBatteryMonitorClass))

#define DALSTON_IS_BATTERY_MONITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DALSTON_TYPE_BATTERY_MONITOR))

#define DALSTON_IS_BATTERY_MONITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DALSTON_TYPE_BATTERY_MONITOR))

#define DALSTON_BATTERY_MONITOR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DALSTON_TYPE_BATTERY_MONITOR, DalstonBatteryMonitorClass))

typedef struct {
  GObject parent;
} DalstonBatteryMonitor;

typedef struct {
  GObjectClass parent_class;
} DalstonBatteryMonitorClass;

typedef enum
{
  DALSTON_BATTERY_MONITOR_STATE_UNKNOWN,
  DALSTON_BATTERY_MONITOR_STATE_CHARGING,
  DALSTON_BATTERY_MONITOR_STATE_DISCHARGING,
  DALSTON_BATTERY_MONITOR_STATE_OTHER
} DalstonBatteryMonitorState;

GType dalston_battery_monitor_get_type (void);

gint dalston_battery_monitor_get_time_remaining (DalstonBatteryMonitor *monitor);
gint dalston_battery_monitor_get_charge_percentage (DalstonBatteryMonitor *monitor);

DalstonBatteryMonitorState dalston_battery_monitor_get_state (DalstonBatteryMonitor *monitor);
gboolean dalston_battery_monitor_get_ac_connected (DalstonBatteryMonitor *monitor);
G_END_DECLS

#endif /* _DALSTON_BATTERY_MONITOR */

