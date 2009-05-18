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


#include <libhal-power-glib/hal-power-proxy.h>

static void
_power_proxy_suspend_cb (HalPowerProxy *proxy,
                         const GError  *error,
                         GObject       *weak_object,
                         gpointer       userdata)
{
  if (error)
  {
    g_warning (G_STRLOC ": Error when shutting down: %s",
               error->message);
  }
}

int
main (int    argc,
      char **argv)
{
  GMainLoop *loop;
  HalPowerProxy *proxy;

  g_type_init ();

  loop = g_main_loop_new (NULL, TRUE);
  proxy = hal_power_proxy_new ();
  hal_power_proxy_suspend (proxy,
                           _power_proxy_suspend_cb,
                           NULL,
                           NULL);
  g_main_loop_run (loop);
}
