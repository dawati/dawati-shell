/*
 * Anerley - people feeds and widgets
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
#include <glib.h>
#include <anerley/anerley-tp-observer.h>
#include <dbus/dbus-glib.h>
#include <telepathy-glib/channel.h>

static void
_observer_new_channel_cb (AnerleyTpObserver *observer,
                          const gchar       *account_name,
                          TpChannel         *channel,
                          gpointer           userdata)
{
  g_debug (G_STRLOC ": New channel for %s on %s",
           tp_channel_get_identifier (channel),
           account_name);
}

int
main (int    argc,
      char **argv)
{
  GMainLoop *main_loop;
  DBusGConnection *conn;
  AnerleyTpObserver *observer;

  g_type_init ();

  main_loop = g_main_loop_new (NULL, FALSE);
  conn = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);

  observer = anerley_tp_observer_new ("AnerleyTest");

  g_signal_connect (observer,
                    "new-channel",
                    (GCallback)_observer_new_channel_cb,
                    NULL);
  g_main_loop_run (main_loop);

  return 0;
}
