
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
#include <clutter-gtk/clutter-gtk.h>
#include <gdk/gdkx.h>
#include <X11/XF86keysym.h>
#include "mpd-global-key.h"

static void
_brightness_up_key_activated_cb (MxAction *action,
                                 void     *data)
{
  g_debug ("%s()", __FUNCTION__);
}

static void
_brightness_down_key_activated_cb (MxAction *action,
                                   void     *data)
{
  g_debug ("%s()", __FUNCTION__);
}

int
main (int     argc,
      char  **argv)
{
  MxAction      *brightness_up_key = NULL;
  MxAction      *brightness_down_key = NULL;
  unsigned int   key_code;

  gtk_clutter_init (&argc, &argv);

  /* Brightness keys. */
  key_code = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_MonBrightnessUp);
  if (key_code)
  {
    brightness_up_key = mpd_global_key_new (key_code);
    g_object_ref_sink (brightness_up_key);
    g_signal_connect (brightness_up_key, "activated",
                      G_CALLBACK (_brightness_up_key_activated_cb), NULL);
  } else {
    g_warning ("Failed to query XF86XK_MonBrightnessUp key code.");
  }

  key_code = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_MonBrightnessDown);
  if (key_code)
  {
    brightness_down_key = mpd_global_key_new (key_code);
    g_object_ref_sink (brightness_down_key);
    g_signal_connect (brightness_down_key, "activated",
                      G_CALLBACK (_brightness_down_key_activated_cb), NULL);
  } else {
    g_warning ("Failed to query XF86XK_MonBrightnessDown key code.");
  }

  clutter_main ();
  g_object_unref (brightness_up_key);
  g_object_unref (brightness_down_key);
  return EXIT_SUCCESS;
}

