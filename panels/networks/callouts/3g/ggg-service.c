/*
 * Carrick - a connection panel for the MeeGo Netbook
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Written by - Ross Burton <ross@linux.intel.com>
 */

#include <config.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "carrick/connman-service-bindings.h"
#include "ggg-service.h"

#define CONNMAN_SERVICE           "net.connman"
#define CONNMAN_MANAGER_PATH      "/"
#define CONNMAN_MANAGER_INTERFACE   CONNMAN_SERVICE ".Manager"
#define CONNMAN_SERVICE_INTERFACE CONNMAN_SERVICE ".Service"

struct _GggServicePrivate {
  DBusGProxy *proxy;
  char *name;
  char *mcc, *mnc;
  gboolean roaming;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GGG_TYPE_SERVICE, GggServicePrivate))
G_DEFINE_TYPE (GggService, ggg_service, G_TYPE_OBJECT);

static void
ggg_service_class_init (GggServiceClass *klass)
{
  g_type_class_add_private (klass, sizeof (GggServicePrivate));
}

static void
ggg_service_init (GggService *self)
{
  self->priv = GET_PRIVATE (self);
}

static const char *
get_string_prop (GHashTable *props, const char *name)
{
  GValue *value;

  value = g_hash_table_lookup (props, name);
  if (value == NULL)
    return NULL;

  if (!G_VALUE_HOLDS_STRING (value)) {
    g_warning ("Property %s is not a string", name);
    return NULL;
  }

  return g_value_get_string (value);
}

static gboolean
get_bool_prop (GHashTable *props, const char *name)
{
  GValue *value;

  value = g_hash_table_lookup (props, name);
  if (value == NULL)
    return FALSE;

  if (!G_VALUE_HOLDS_BOOLEAN (value)) {
    g_warning ("Property %s is not a boolean", name);
    return FALSE;
  }

  return g_value_get_boolean (value);
}

GggService *
ggg_service_new (DBusGConnection *connection, const char *path)
{
  GggService *service;
  DBusGProxy *proxy;
  GHashTable *props;
  GError *error = NULL;
  const char *s;

  g_assert (connection);
  g_assert (path);

  proxy = dbus_g_proxy_new_for_name (connection,
                                     CONNMAN_SERVICE,
                                     path,
                                     CONNMAN_SERVICE_INTERFACE);

  if (!net_connman_Service_get_properties (proxy, &props, &error)) {
    g_printerr ("Cannot get properties for service: %s\n", error->message);
    g_error_free (error);
    g_object_unref (proxy);
    return NULL;
  }

  s = get_string_prop (props, "Type");
  if (s == NULL) {
    g_hash_table_unref (props);
    g_object_unref (proxy);
    return NULL;
  }

  if (g_strcmp0 (s, "cellular") != 0) {
    g_hash_table_unref (props);
    g_object_unref (proxy);
    return NULL;
  }

  service = g_object_new (GGG_TYPE_SERVICE, NULL);
  service->priv->proxy = proxy;

  service->priv->name = g_strdup (get_string_prop (props, "Name"));
  service->priv->mcc = g_strdup (get_string_prop (props, "MCC"));
  service->priv->mnc = g_strdup (get_string_prop (props, "MNC"));
  service->priv->roaming = get_bool_prop (props, "Roaming");

  g_hash_table_unref (props);

  return service;
}

GggService *
ggg_service_new_fake (const char *name, gboolean roaming)
{
  GggService *service;

  service = g_object_new (GGG_TYPE_SERVICE, NULL);
  service->priv->proxy = NULL;

  service->priv->name = g_strdup (name);
  service->priv->mcc = g_strdup ("234");
  service->priv->mnc = g_strdup ("15");
  service->priv->roaming = roaming;

  return service;
}

const char *
ggg_service_get_name (GggService *service)
{
  return service->priv->name;
}

gboolean
ggg_service_is_roaming (GggService *service)
{
  return service->priv->roaming;
}

const char *
ggg_service_get_mcc (GggService *service)
{
  return service->priv->mcc;
}

const char *
ggg_service_get_mnc (GggService *service)
{
  return service->priv->mnc;
}

static void
set_string_prop (DBusGProxy *proxy, const char *name, const char *string)
{
  GError *error = NULL;
  GValue value = { 0, };

  if (string == NULL)
    return;

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_static_string (&value, string);

  if (!net_connman_Service_set_property (proxy, name, &value, &error)) {
    g_printerr ("Cannot set property %s to %s: %s\n",
                name, string, error->message);
    g_error_free (error);
  }

  g_value_unset (&value);
}

void
ggg_service_set (GggService *service,
                 const char *apn,
                 const char *username,
                 const char *password)
{
  if (service->priv->proxy) {
    set_string_prop (service->priv->proxy, "APN", apn);
    set_string_prop (service->priv->proxy, "Username", username);
    set_string_prop (service->priv->proxy, "Password", password);
  } else {
    g_debug ("ggg_service_set: APN %s, username %s, password %s",
             apn, username, password);
  }
}

void
ggg_service_connect (GggService *service)
{
  GError *error = NULL;

  if (service->priv->proxy) {
    if (!net_connman_Service_connect (service->priv->proxy, &error)) {
      g_printerr ("Cannot connect: %s\n", error->message);
      g_error_free (error);
    }
  } else {
    g_debug ("ggg_service_connect");
  }
}
