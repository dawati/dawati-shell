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


#include "dalston-battery-monitor.h"

#include <libhal-glib/hal-manager.h>
#include <libhal-glib/hal-device.h>

G_DEFINE_TYPE (DalstonBatteryMonitor, dalston_battery_monitor, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_BATTERY_MONITOR, DalstonBatteryMonitorPrivate))

typedef struct _DalstonBatteryMonitorPrivate DalstonBatteryMonitorPrivate;

struct _DalstonBatteryMonitorPrivate {
  HalManager *manager;
  HalDevice  *battery_device;
  HalDevice  *ac_device;
  gboolean    is_hal_running;

  gchar *battery_udi;
  gchar *ac_udi;
};

enum
{
  STATUS_CHANGED,
  LAST_SIGNAL
};

guint signals[LAST_SIGNAL];

static void
dalston_battery_monitor_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_battery_monitor_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_battery_monitor_dispose (GObject *object)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (object);

  if (priv->manager)
  {
    g_object_unref (priv->manager);
    priv->manager = NULL;
  }

  if (priv->battery_device)
  {
    g_object_unref (priv->battery_device);
    priv->battery_device = NULL;
  }

  if (priv->ac_device)
  {
    g_object_unref (priv->ac_device);
    priv->ac_device = NULL;
  }

  G_OBJECT_CLASS (dalston_battery_monitor_parent_class)->dispose (object);
}

static void
dalston_battery_monitor_finalize (GObject *object)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (object);

  g_free (priv->battery_udi);
  g_free (priv->ac_udi);

  G_OBJECT_CLASS (dalston_battery_monitor_parent_class)->finalize (object);
}

static void
dalston_battery_monitor_class_init (DalstonBatteryMonitorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DalstonBatteryMonitorPrivate));

  object_class->get_property = dalston_battery_monitor_get_property;
  object_class->set_property = dalston_battery_monitor_set_property;
  object_class->dispose = dalston_battery_monitor_dispose;
  object_class->finalize = dalston_battery_monitor_finalize;

  signals[STATUS_CHANGED] = g_signal_new ("status-changed",
                                          DALSTON_TYPE_BATTERY_MONITOR,
                                          G_SIGNAL_RUN_FIRST,
                                          0,
                                          NULL,
                                          NULL,
                                          g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE,
                                          0);
}

static void
_hal_battery_device_property_modified (HalDevice   *device,
                                       const gchar *key,
                                       gboolean     is_added,
                                       gboolean     is_removed,
                                       gboolean     finally,
                                       gpointer     userdata)
{
  DalstonBatteryMonitor *monitor = DALSTON_BATTERY_MONITOR (userdata);
  const gchar *key_name;

  key_name = g_intern_string (key);

  if ((key_name == g_intern_static_string ("battery.charge_level.percentage")) ||
      (key_name == g_intern_static_string ("battery.remaining_time")) ||
      (key_name == g_intern_static_string ("battery.rechargeable.is_charging")) ||
      (key_name == g_intern_static_string ("battery.rechargeable.is_discharging")))
  {
    g_signal_emit (monitor, signals[STATUS_CHANGED], 0);
  }
}

static void
_hal_ac_device_property_modified (HalDevice   *device,
                                  const gchar *key,
                                  gboolean     is_added,
                                  gboolean     is_removed,
                                  gboolean     finally,
                                  gpointer     userdata)
{
  DalstonBatteryMonitor *monitor = DALSTON_BATTERY_MONITOR (userdata);
  const gchar *key_name;

  key_name = g_intern_string (key);

  if (key_name == g_intern_static_string ("ac_adapter.present"))
  {
    g_signal_emit (monitor, signals[STATUS_CHANGED], 0);
  }
}

static void
_setup_battery_device (DalstonBatteryMonitor *self)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (self);
  gchar **names;
  GError *error = NULL;

  if (!hal_manager_find_capability (priv->manager,
                                    "battery",
                                    &names,
                                    &error))
  {
    g_warning (G_STRLOC ": Unable to find device with capability 'battery': %s",
               error->message);
    g_clear_error (&error);
    return;
  }

  priv->battery_udi = g_strdup (names[0]);
  hal_manager_free_capability (names);

  if (priv->battery_udi)
  {
    priv->battery_device = hal_device_new ();
    hal_device_set_udi (priv->battery_device, priv->battery_udi);
    hal_device_watch_property_modified (priv->battery_device);
    g_signal_connect (priv->battery_device,
                      "property-modified",
                      (GCallback)_hal_battery_device_property_modified,
                      self);
  }
}

static void
_cleanup_battery_device (DalstonBatteryMonitor *self)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (self);

  if (priv->battery_device)
  {
    g_object_unref (priv->battery_device);
    priv->battery_device = NULL;
  }
  g_free (priv->battery_udi);
}


static void
_setup_ac_device (DalstonBatteryMonitor *self)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (self);
  gchar **names;
  GError *error = NULL;

  if (!hal_manager_find_capability (priv->manager,
                                    "ac_adapter",
                                    &names,
                                    &error))
  {
    g_warning (G_STRLOC ": Unable to find device with capability 'ac_adapter': %s",
               error->message);
    g_clear_error (&error);
    return;
  }

  priv->ac_udi = g_strdup (names[0]);
  hal_manager_free_capability (names);

  if (priv->ac_udi)
  {
    priv->ac_device = hal_device_new ();
    hal_device_set_udi (priv->ac_device, priv->ac_udi);
    hal_device_watch_property_modified (priv->ac_device);
    g_signal_connect (priv->ac_device,
                      "property-modified",
                      (GCallback)_hal_ac_device_property_modified,
                      self);
  }

}

static void
_cleanup_ac_device (DalstonBatteryMonitor *self)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (self);

  if (priv->ac_device)
  {
    g_object_unref (priv->ac_device);
    priv->ac_device = NULL;
  }
  g_free (priv->ac_udi);
}

static void
_hal_daemon_start (HalManager            *manager,
                   DalstonBatteryMonitor *self)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (self);

  /* spurious signal ? */
  if (priv->is_hal_running)
    return;

  _setup_battery_device (self);
  _setup_ac_device (self);

  priv->is_hal_running = TRUE;

  g_signal_emit (self, signals[STATUS_CHANGED], 0);
}

static void
_hal_daemon_stop (HalManager            *manager,
                  DalstonBatteryMonitor *self)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (self);

  /* spurious signal ? */
  if (!priv->is_hal_running)
    return;

  _cleanup_battery_device (self);
  _cleanup_ac_device (self);

  priv->is_hal_running = FALSE;

  g_signal_emit (self, signals[STATUS_CHANGED], 0);
}

static void
_hal_device_added_cb (HalManager  *manager,
                      const gchar *udi,
                      gpointer     userdata)
{
	DalstonBatteryMonitor *monitor = DALSTON_BATTERY_MONITOR (userdata);
  gboolean is_battery;
  HalDevice *device;

  device = hal_device_new ();
  hal_device_set_udi (device, udi);
  hal_device_query_capability (device, "battery", &is_battery, NULL);
  if (is_battery == TRUE)
  {
    _setup_battery_device (monitor);
    g_signal_emit (monitor, signals[STATUS_CHANGED], 0);
  }
  g_object_unref (device);
}

static void
_hal_device_removed_cb (HalManager  *manager,
                        const gchar *udi,
                        gpointer     userdata)
{
	DalstonBatteryMonitor *monitor = DALSTON_BATTERY_MONITOR (userdata);
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (monitor);

  if (g_strcmp0 (priv->battery_udi, udi) == 0)
  {
    _cleanup_battery_device (monitor);
    g_signal_emit (monitor, signals[STATUS_CHANGED], 0);
  }
}

static void
dalston_battery_monitor_init (DalstonBatteryMonitor *self)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (self);

  priv->manager = hal_manager_new ();
  g_signal_connect (priv->manager,
                    "daemon-start",
                    G_CALLBACK (_hal_daemon_start),
                    self);
  g_signal_connect (priv->manager,
                    "daemon-stop",
                    G_CALLBACK (_hal_daemon_stop),
                    self);
  g_signal_connect (priv->manager,
                    "device-added",
                    G_CALLBACK (_hal_device_added_cb),
                    self);
  g_signal_connect (priv->manager,
                    "device-removed",
                    G_CALLBACK (_hal_device_removed_cb),
                    self);

  priv->is_hal_running = FALSE;
  if (hal_manager_is_running (priv->manager))
  {
    priv->is_hal_running = TRUE;
    _setup_battery_device (self);
    _setup_ac_device (self);
  }
}

gint
dalston_battery_monitor_get_time_remaining (DalstonBatteryMonitor *monitor)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (monitor);
  gint value = -1;
  GError *error = NULL;

  if (priv->battery_device)
  {
    if (!hal_device_get_int (priv->battery_device,
                             "battery.remaining_time",
                             &value,
                             &error))
    {
      if (error)
      {
        g_warning (G_STRLOC ": Error getting time remaining: %s",
                   error->message);
        g_clear_error (&error);
      }
    }
  }

  return value;
}

gint
dalston_battery_monitor_get_charge_percentage (DalstonBatteryMonitor *monitor)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (monitor);
  gint value = -1;
  GError *error = NULL;

  if (priv->battery_device)
  {
    if (!hal_device_get_int (priv->battery_device,
                             "battery.charge_level.percentage",
                             &value,
                             &error))
    {
      if (error)
      {
        g_warning (G_STRLOC ": Error getting charge percentage: %s",
                   error->message);
        g_clear_error (&error);
      }
    }
  }

  return value;
}

DalstonBatteryMonitorState
dalston_battery_monitor_get_state (DalstonBatteryMonitor *monitor)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (monitor);
  DalstonBatteryMonitorState state = DALSTON_BATTERY_MONITOR_STATE_MISSING;
  gboolean value = FALSE;
  GError *error = NULL;

  /* for some valid reason (HAL not running or HAL running but the battery is
   * actually missing) battery_device is null */
  if (priv->battery_device == NULL)
    return DALSTON_BATTERY_MONITOR_STATE_MISSING;

  if (!hal_device_get_bool (priv->battery_device,
                            "battery.rechargeable.is_charging",
                            &value,
                            &error))
  {
    if (error)
    {
      g_warning (G_STRLOC ": Error getting charge is_charging: %s",
                 error->message);
      g_clear_error (&error);
    }

    return state;
  }

  if (value)
    return DALSTON_BATTERY_MONITOR_STATE_CHARGING;

  if (!hal_device_get_bool (priv->battery_device,
                            "battery.rechargeable.is_discharging",
                            &value,
                            &error))
  {
    if (error)
    {
      g_warning (G_STRLOC ": Error getting charge is_discharging: %s",
                 error->message);
      g_clear_error (&error);
    }

    return state;
  }

  if (value)
    return DALSTON_BATTERY_MONITOR_STATE_DISCHARGING;

  return DALSTON_BATTERY_MONITOR_STATE_OTHER;
}

gboolean
dalston_battery_monitor_get_ac_connected (DalstonBatteryMonitor *monitor)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (monitor);
  gboolean value = FALSE;
  GError *error = NULL;

  if (priv->ac_device)
  {
    if (!hal_device_get_bool (priv->ac_device,
                              "ac_adapter.present",
                              &value,
                              &error))
    {
      if (error)
      {
        g_warning (G_STRLOC ": Error getting value ac_adapter.present: %s",
                   error->message);
        g_clear_error (&error);
      }
    }
  }

  return value;
}
