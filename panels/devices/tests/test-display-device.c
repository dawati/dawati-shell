
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
#include <clutter-gtk/clutter-gtk.h>
#include "mpd-display-device.h"

static void
_display_enabled_notify_cb (MpdDisplayDevice  *display,
                            GParamSpec        *pspec,
                            void              *data)
{
  g_debug ("enabled: %d", mpd_display_device_is_enabled (display));
}

static void
_display_brightness_notify_cb (MpdDisplayDevice  *display,
                               GParamSpec        *pspec,
                               void              *data)
{
  g_debug ("brightness: %.1f", mpd_display_device_get_brightness (display));
}

static bool
_brightness_down_cb (MpdDisplayDevice *display)
{
  float brightness;

  brightness = mpd_display_device_get_brightness (display);
  g_debug ("%s() %.3f", __FUNCTION__, brightness);

  mpd_display_device_set_brightness (display, brightness - 0.1);

  return false;
}

static bool
_brightness_up_cb (MpdDisplayDevice *display)
{
  float brightness;

  brightness = mpd_display_device_get_brightness (display);
  g_debug ("%s() %.3f", __FUNCTION__, brightness);

  mpd_display_device_set_brightness (display, brightness + 0.1);

  return false;
}

int
main (int     argc,
      char  **argv)
{
  gboolean down = false;
  gboolean up = false;
  GOptionEntry _options[] = {
    { "brightness-down", 'd', 0, G_OPTION_ARG_NONE, &down,
      "Decrease screen brightness by one step", NULL },
    { "brightness-up", 'u', 0, G_OPTION_ARG_NONE, &up,
      "Increase screen brightness by one step", NULL },
    { NULL }
  };

  MpdDisplayDevice  *display;
  GOptionContext    *context;
  GError            *error = NULL;

  context = g_option_context_new ("");
  g_option_context_add_main_entries (context, _options, NULL);
  g_option_context_add_group (context, clutter_get_option_group_without_init ());
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical ("%s %s", G_STRLOC, error->message);
    return EXIT_FAILURE;
  }
  g_option_context_free (context);

  gtk_clutter_init (&argc, &argv);

  display = mpd_display_device_new ();
  g_debug ("enabled: %i, brightness, %.1f",
           mpd_display_device_is_enabled (display),
           mpd_display_device_get_brightness (display));

  g_signal_connect (display, "notify::enabled",
                    G_CALLBACK (_display_enabled_notify_cb), NULL);
  g_signal_connect (display, "notify::brightness",
                    G_CALLBACK (_display_brightness_notify_cb), NULL);

  if (up)
  {
    g_idle_add ((GSourceFunc) _brightness_up_cb, display);
  } else if (down)
  {
    g_idle_add ((GSourceFunc) _brightness_down_cb, display);
  }

  clutter_main ();
  g_object_unref (display);
  return EXIT_SUCCESS;
}
