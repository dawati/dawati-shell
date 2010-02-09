
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Robert Staudinger <robert.staudinger@intel.com>
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

#include <stdlib.h>
#include <clutter/clutter.h>
#include "mpd-battery-device.h"

static void
battery_print (MpdBatteryDevice *battery)
{
  g_debug ("percentage: %f", mpd_battery_device_get_percentage (battery));
  g_debug ("state: %s", mpd_battery_device_get_state_text (battery));
}

static void
_battery_notify_cb (MpdBatteryDevice  *battery,
                    GParamSpec        *pspec,
                    gpointer           user_data)
{
  battery_print (battery);
}

int
main (int     argc,
      char  **argv)
{
  MpdBatteryDevice *battery;

  clutter_init (&argc, &argv);

  battery = mpd_battery_device_new ();
  g_signal_connect (battery, "notify::percentage",
                    G_CALLBACK (_battery_notify_cb), NULL);
  g_signal_connect (battery, "notify::state",
                    G_CALLBACK (_battery_notify_cb), NULL);
  battery_print (battery);

  clutter_main ();
  g_object_unref (battery);
  return EXIT_SUCCESS;
}
