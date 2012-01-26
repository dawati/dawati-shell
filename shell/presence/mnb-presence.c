
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Ross Burton <ross@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include "../dawati-netbook.h"
#include "gsm-manager.h"

static void
connect_to_dbus (void)
{
  DBusGConnection *connection;
  DBusGProxy *bus_proxy;
  GError *error = NULL;
  guint32 request_name_ret;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection) {
    g_warning ("Cannot connect to DBus: %s", error->message);
    g_error_free (error);
    return;
  }

  bus_proxy = dbus_g_proxy_new_for_name (connection,
                                         DBUS_SERVICE_DBUS,
                                         DBUS_PATH_DBUS,
                                         DBUS_INTERFACE_DBUS);

  if (!org_freedesktop_DBus_request_name (bus_proxy,
                                          "org.gnome.SessionManager",
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &request_name_ret, &error))
    {
      g_warning ("Cannot request name: %s", error->message);
      g_error_free (error);
      return;
    }

  if (request_name_ret == DBUS_REQUEST_NAME_REPLY_EXISTS)
    {
      g_printerr ("Presence manager already running, not taking over\n");
      return;
    }

  g_object_unref (bus_proxy);
}

void
presence_init (DawatiNetbookPlugin *plugin)
{
  connect_to_dbus ();

  plugin->priv->manager = gsm_manager_new ();
}
