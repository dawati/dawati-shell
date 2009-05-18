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


#include "dalston-button-monitor.h"

#include <libhal-glib/hal-manager.h>
#include <libhal-glib/hal-device.h>
#include <libhal-power-glib/hal-power-proxy.h>

G_DEFINE_TYPE (DalstonButtonMonitor, dalston_button_monitor, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_BUTTON_MONITOR, DalstonButtonMonitorPrivate))

typedef struct _DalstonButtonMonitorPrivate DalstonButtonMonitorPrivate;

struct _DalstonButtonMonitorPrivate {
  HalPowerProxy *power_proxy;
  HalManager *manager;
  GList *devices;
};

static void
dalston_button_monitor_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_button_monitor_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_button_monitor_dispose (GObject *object)
{
  DalstonButtonMonitorPrivate *priv = GET_PRIVATE (object);
  GList *l = NULL;
  HalDevice *device = NULL;

  if (priv->power_proxy)
  {
    g_object_unref (priv->power_proxy);
    priv->power_proxy = NULL;
  }

  if (priv->manager)
  {
    g_object_unref (priv->manager);
    priv->manager = NULL;
  }

  for (l = priv->devices; l; l = g_list_delete_link (l, l))
  {
    device = (HalDevice *)l->data;

    if (device)
    {
      g_object_unref (device);
    }
  }

  priv->devices = l;

  G_OBJECT_CLASS (dalston_button_monitor_parent_class)->dispose (object);
}

static void
dalston_button_monitor_finalize (GObject *object)
{
  G_OBJECT_CLASS (dalston_button_monitor_parent_class)->finalize (object);
}

static void
dalston_button_monitor_class_init (DalstonButtonMonitorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DalstonButtonMonitorPrivate));

  object_class->get_property = dalston_button_monitor_get_property;
  object_class->set_property = dalston_button_monitor_set_property;
  object_class->dispose = dalston_button_monitor_dispose;
  object_class->finalize = dalston_button_monitor_finalize;
}


static void
_device_condition_cb (HalDevice   *device,
                      const gchar *condition,
                      const gchar *details,
                      gpointer     userdata)
{
  DalstonButtonMonitor *monitor = (DalstonButtonMonitor *)userdata;
  DalstonButtonMonitorPrivate *priv = GET_PRIVATE (monitor);
  gchar *type = NULL;
  GError *error = NULL;

  if (!g_str_equal (condition, "ButtonPressed"))
  {
    /* Not expecting another event...*/
    return;
  }

  if (!hal_device_get_string (device,
                              "button.type",
                              &type,
                              &error))
  {
    g_warning (G_STRLOC ": Getting type failed: %s",
               error->message);
    g_clear_error (&error);
    g_free (type);
    return;
  }

  if (g_str_equal (type, "sleep") || g_str_equal (type, "lid"))
  {
    hal_power_proxy_suspend (priv->power_proxy,
                             NULL,
                             NULL,
                             NULL);
  }

  g_free (type);
}

static void
dalston_button_monitor_init (DalstonButtonMonitor *self)
{
  DalstonButtonMonitorPrivate *priv = GET_PRIVATE (self);
  gchar **names;
  GError *error = NULL;
  gchar *udi;
  gint i;
  HalDevice *device;
  gchar *type;

  priv->power_proxy = hal_power_proxy_new ();

  if (!priv->power_proxy)
  {
    g_warning (G_STRLOC ": No power proxy; not listening for buttons.");
    return;
  }

  priv->manager = hal_manager_new ();

  if (!hal_manager_find_capability (priv->manager,
                                    "button",
                                    &names,
                                    &error))
  {
    g_warning (G_STRLOC ": Unable to find devices with capability 'button': %s",
               error->message);
    g_clear_error (&error);
    return;
  }

  for (i = 0, udi = names[i]; udi != NULL; i++, udi = names[i])
  {
    type = NULL;

    device = hal_device_new ();
    hal_device_set_udi (device, udi);

    if (!hal_device_get_string (device,
                                "button.type",
                                &type,
                                NULL))
    {
      /* It's perfectly fine for a button not to have a type */
      g_free (type);
      g_object_unref (device);
      device = NULL;
      continue;
    }

    /* Okay so we have a type now, let's see what it is.. */

    if (g_str_equal (type, "power") ||
        g_str_equal (type, "sleep") ||
        g_str_equal (type, "lid"))
    {
      hal_device_watch_condition (device);
      g_signal_connect (device,
                        "device-condition",
                        (GCallback)_device_condition_cb,
                        self);
      priv->devices = g_list_append (priv->devices, device);
    } else {
      /* Some other button type. Weird. So let's skiparrooo */
      g_free (type);
      g_object_unref (device);
      device = NULL;
      continue;
    }
  }

  hal_manager_free_capability (names);
}

DalstonButtonMonitor *
dalston_button_monitor_new (void)
{
  return g_object_new (DALSTON_TYPE_BUTTON_MONITOR, NULL);
}


