
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

#include <stdbool.h>
#include <stdlib.h>
#include <clutter/clutter.h>
#include "mpd-display-device.h"

static void
_display_enabled_notify_cb (MpdDisplayDevice  *display,
                            GParamSpec        *pspec,
                            void              *data)
{
  g_debug ("enabled: %d", mpd_display_device_is_enabled (display, NULL));
}

static void
_display_percentage_notify_cb (MpdDisplayDevice  *display,
                               GParamSpec        *pspec,
                               void              *data)
{
  g_debug ("percentage: %d", mpd_display_device_get_percentage (display, NULL));
}

int
main (int     argc,
      char  **argv)
{
  MpdDisplayDevice *display;

  clutter_init (&argc, &argv);

  display = mpd_display_device_new ();
  g_debug ("enabled: %i, percentage, %i",
           mpd_display_device_is_enabled (display, NULL),
           mpd_display_device_get_percentage (display, NULL));

  g_signal_connect (display, "notify::enabled",
                    G_CALLBACK (_display_enabled_notify_cb), NULL);
  g_signal_connect (display, "notify::percentage",
                    G_CALLBACK (_display_percentage_notify_cb), NULL);

  clutter_main ();
  g_object_unref (display);
  return EXIT_SUCCESS;
}
