/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * carrick-proxy-to-gsettings - updates the global gnome proxy settings with
 * the service currently being use.
 *
 * Copyright © 2012 Intel Corporation.
 *
 * Author: Michael Wood <michael.g.wood@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include <stdlib.h>

static void
_gsettings_unset_proxy (GSettings *proxy_settings)
{
  GSettings *http_settings;
  GSettings *https_settings;

  http_settings = g_settings_get_child (proxy_settings, "http");
  https_settings = g_settings_get_child (proxy_settings, "https");

  g_settings_set_string (proxy_settings, "mode", "none");

  g_settings_set_boolean (http_settings, "enabled", FALSE);
  g_settings_set_string (http_settings, "host", "");
  g_settings_set_int (http_settings, "port", 0);

  g_settings_set_string (https_settings, "host", "");
  g_settings_set_int (https_settings, "port", 0);

  g_object_unref (http_settings);
  g_object_unref (https_settings);
}

static void
_gsettings_set_proxy (GSettings *proxy_settings, const gchar *proxy)
{
  /* 0-host 1-port */
  gchar **hostandport = NULL;
  guint port;

  GSettings *http_settings;
  GSettings *https_settings;

  if (!proxy)
    return;

  hostandport = g_strsplit ((proxy + strlen ("http://")), ":", 2);

  if (g_strv_length (hostandport) > 1)
    port = atoi (hostandport[1]);
  else
    port = 8080;

  http_settings = g_settings_get_child (proxy_settings, "http");
  https_settings = g_settings_get_child (proxy_settings, "https");

  g_settings_set_string (proxy_settings, "mode", "manual");

  g_settings_set_boolean (http_settings, "enabled", TRUE);
  g_settings_set_string (http_settings, "host", hostandport[0]);
  g_settings_set_int (http_settings, "port", port);

  g_settings_set_string (https_settings, "host", hostandport[0]);
  g_settings_set_int (https_settings, "port", port);

  if (hostandport)
    g_strfreev (hostandport);

  g_object_unref (http_settings);
  g_object_unref (https_settings);
}

void carrick_proxy_gsettings_update_proxy (void)
{
  gchar **proxies;
  GProxyResolver *res;
  GSettings *proxy_settings;

  proxy_settings = g_settings_new ("org.gnome.system.proxy");

  _gsettings_unset_proxy (proxy_settings);

  res = g_proxy_resolver_get_default ();

  if (res)
    {
      proxies = g_proxy_resolver_lookup (res, "http://.", NULL, NULL);

      if (proxies && g_strcmp0 (proxies[0], "direct://") != 0)
        {
          _gsettings_set_proxy (proxy_settings, proxies[0]);
        }

      if (proxies)
        g_strfreev (proxies);
    }

  g_object_unref (proxy_settings);
}
