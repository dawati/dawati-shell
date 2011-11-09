
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
#include "mpd-battery-device.h"

static void
battery_print (MpdBatteryDevice *battery,
               bool              raw)
{
  if (raw)
  {
    mpd_battery_device_dump (battery);
  } else {
    char *text = mpd_battery_device_get_state_text (battery);
    g_debug ("%s", text);
    g_free (text);
  }
}

static void
_battery_notify_cb (MpdBatteryDevice  *battery,
                    GParamSpec        *pspec,
                    bool               raw)
{
  battery_print (battery, raw);
}

int
main (int     argc,
      char  **argv)
{
  bool raw = false;
  GOptionEntry _options[] = {
    { "raw", 'r', 0, G_OPTION_ARG_NONE, &raw,
      "Display raw battery information", NULL },
    { NULL }
  };

  MpdBatteryDevice  *battery;
  GOptionContext    *context;
  GError            *error = NULL;

  context = g_option_context_new ("- Test battery device");
  g_option_context_add_main_entries (context, _options, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical ("%s %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
  g_option_context_free (context);

  clutter_init (&argc, &argv);

  battery = mpd_battery_device_new ();
  g_signal_connect (battery, "notify::percentage",
                    G_CALLBACK (_battery_notify_cb), (void *) raw);
  g_signal_connect (battery, "notify::state",
                    G_CALLBACK (_battery_notify_cb), (void *) raw);
  battery_print (battery, raw);

  clutter_main ();
  g_object_unref (battery);
  return EXIT_SUCCESS;
}
