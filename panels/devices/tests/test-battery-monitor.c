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


#include <dalston/dalston-battery-monitor.h>

static void
_battery_monitor_status_changed_cb (DalstonBatteryMonitor *monitor,
                                    gpointer               userdata)
{
  g_debug (G_STRLOC ": Status changed");
  g_debug (G_STRLOC ": Remaining time: %d. Remaining percentage: %d",
           dalston_battery_monitor_get_time_remaining (monitor),
           dalston_battery_monitor_get_charge_percentage (monitor));
}

int
main (int    argc,
      char **argv)
{
  GMainLoop *loop;
  DalstonBatteryMonitor *monitor;

  g_type_init ();
  loop = g_main_loop_new (NULL, TRUE);

  monitor = g_object_new (DALSTON_TYPE_BATTERY_MONITOR,
                          NULL);

  g_signal_connect (monitor,
                    "status-changed",
                    _battery_monitor_status_changed_cb,
                    NULL);

  g_main_loop_run (loop);
}
