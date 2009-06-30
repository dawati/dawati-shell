/*
 * libhal-panel-proxy - library for accessing HAL's PowerManagement interface
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


#include "hal-power-proxy.h"
#include "hal-system-power-management-bindings.h"

G_DEFINE_TYPE (HalPowerProxy, hal_power_proxy, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAL_POWER_TYPE_PROXY, HalPowerProxyPrivate))

typedef struct _HalPowerProxyPrivate HalPowerProxyPrivate;

struct _HalPowerProxyPrivate {
  DBusGProxy *proxy;
  DBusGConnection *connection;

};

#define HAL_DBUS_NAME "org.freedesktop.Hal"
#define HAL_DBUS_PANEL_INTERFACE "org.freedesktop.Hal.Device.SystemPowerManagement"

#define COMPUTER_DEVICE_PATH "/org/freedesktop/Hal/devices/computer"

static void
hal_power_proxy_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
hal_power_proxy_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
hal_power_proxy_dispose (GObject *object)
{
  HalPowerProxyPrivate *priv = GET_PRIVATE (object);

  if (priv->proxy)
  {
    g_object_unref (priv->proxy);
    priv->proxy = NULL;
  }

  if (priv->connection)
  {
    dbus_g_connection_unref (priv->connection);
    priv->connection = NULL;
  }

  G_OBJECT_CLASS (hal_power_proxy_parent_class)->dispose (object);
}

static void
hal_power_proxy_finalize (GObject *object)
{
  G_OBJECT_CLASS (hal_power_proxy_parent_class)->finalize (object);
}

static void
hal_power_proxy_class_init (HalPowerProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (HalPowerProxyPrivate));

  object_class->get_property = hal_power_proxy_get_property;
  object_class->set_property = hal_power_proxy_set_property;
  object_class->dispose = hal_power_proxy_dispose;
  object_class->finalize = hal_power_proxy_finalize;
}

static void
hal_power_proxy_init (HalPowerProxy *self)
{
}

typedef struct
{
  HalPowerProxy *proxy;
  HalPowerProxyCallback cb;
  GObject *weak_object;
  gpointer userdata;
  DBusGProxyCall *call;
} CallClosure;

static void 
_weak_notify_cb (gpointer  data,
                 GObject  *dead_object)
{
  CallClosure *closure = (CallClosure *)data;
  HalPowerProxyPrivate *priv = GET_PRIVATE (closure->proxy);

  dbus_g_proxy_cancel_call (priv->proxy, closure->call);
  g_object_unref (closure->proxy);
  g_slice_free (CallClosure, closure);
}

static void
_generic_cb (DBusGProxy *proxy,
              gint        OUT_return_code,
              GError     *error,
              gpointer    userdata)
{
  CallClosure *closure = (CallClosure *)userdata;

  if (closure->cb)
  {
    closure->cb (closure->proxy,
                 error,
                 closure->weak_object,
                 closure->userdata);
  }

  if (closure->weak_object)
  {
    g_object_weak_unref (closure->weak_object, _weak_notify_cb, closure);
  }

  g_object_unref (closure->proxy);
  g_slice_free (CallClosure, closure);
}

void
hal_power_proxy_shutdown (HalPowerProxy         *proxy,
                          HalPowerProxyCallback  cb,
                          GObject               *weak_object,
                          gpointer               userdata)
{
  HalPowerProxyPrivate *priv = GET_PRIVATE (proxy);
  DBusGProxyCall *call;
  CallClosure *closure;

  closure = g_slice_new0 (CallClosure);
  closure->proxy = g_object_ref (proxy);
  closure->cb = cb;
  closure->userdata = userdata;
  closure->weak_object = weak_object;

  if (closure->weak_object)
  {
    g_object_weak_ref (closure->weak_object, _weak_notify_cb, closure);
  }

  call = 
    org_freedesktop_Hal_Device_SystemPowerManagement_shutdown_async (priv->proxy,
                                                                     _generic_cb,
                                                                     closure);

  closure->call = call;
}

void
hal_power_proxy_suspend (HalPowerProxy         *proxy,
                         HalPowerProxyCallback  cb,
                         GObject               *weak_object,
                         gpointer               userdata)
{
  HalPowerProxyPrivate *priv = GET_PRIVATE (proxy);
  DBusGProxyCall *call;
  CallClosure *closure;

  closure = g_slice_new0 (CallClosure);
  closure->proxy = g_object_ref (proxy);
  closure->cb = cb;
  closure->userdata = userdata;
  closure->weak_object = weak_object;

  if (closure->weak_object)
  {
    g_object_weak_ref (closure->weak_object, _weak_notify_cb, closure);
  }

  call = 
    org_freedesktop_Hal_Device_SystemPowerManagement_suspend_async (priv->proxy,
                                                                    0,
                                                                    _generic_cb,
                                                                    closure);

  closure->call = call;
}

void
hal_power_proxy_suspend_sync (HalPowerProxy *proxy)
{
  HalPowerProxyPrivate *priv = GET_PRIVATE (proxy);
  GError *error = NULL;

  org_freedesktop_Hal_Device_SystemPowerManagement_suspend (priv->proxy,
                                                            0,
                                                            NULL,
                                                            &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error suspending: %s",
               error->message);
    g_clear_error (&error);
  }
}

static gboolean
_setup_proxy (HalPowerProxy *proxy,
              const gchar   *device_path)
{
  HalPowerProxyPrivate *priv = GET_PRIVATE (proxy);
  GError *error = NULL;

  priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);

  if (error)
  {
    g_warning (G_STRLOC ": Unable to get bus: %s",
               error->message);
    g_clear_error (&error);
    return FALSE;
  }

  priv->proxy = dbus_g_proxy_new_for_name (priv->connection,
                                           HAL_DBUS_NAME,
                                           device_path,
                                           HAL_DBUS_PANEL_INTERFACE);

  return TRUE;
}


HalPowerProxy *
hal_power_proxy_new (void)
{
  HalPowerProxy *proxy;

  proxy = g_object_new (HAL_POWER_TYPE_PROXY, NULL);

  if (!_setup_proxy (proxy, COMPUTER_DEVICE_PATH))
  {
    g_object_unref (proxy);
    return NULL;
  }

  return proxy;

}


