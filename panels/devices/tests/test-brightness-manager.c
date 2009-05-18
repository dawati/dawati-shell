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


#include <dalston/dalston-brightness-manager.h>

static void
_manager_num_levels_changed_cb (DalstonBrightnessManager *manager,
                                gint                      num_levels)
{
  g_debug (G_STRLOC ": Num levels changed: %d",
           num_levels);
}

static void
_brightness_changed_cb (DalstonBrightnessManager *manager,
                        gint                      value)
{
  g_debug (G_STRLOC ": Brightness: %d", value);
}

int
main (int    argc,
      char **argv)
{
  GMainLoop *loop;
  DalstonBrightnessManager *manager;

  g_type_init ();
  loop = g_main_loop_new (NULL, TRUE);

  manager = dalston_brightness_manager_new ();

  g_signal_connect (manager,
                    "num-levels-changed",
                    _manager_num_levels_changed_cb,
                    NULL);

  dalston_brightness_manager_start_monitoring (manager);

  g_signal_connect (manager,
                    "brightness-changed",
                    _brightness_changed_cb,
                    NULL);
  g_main_loop_run (loop);
}
