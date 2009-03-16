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
    g_debug (G_STRLOC ": Key changed: %s", key);
    g_signal_emit (monitor, signals[STATUS_CHANGED], 0);
  }
}

static void
dalston_battery_monitor_init (DalstonBatteryMonitor *self)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (self);
  gchar **names;
  GError *error = NULL;

  priv->manager = hal_manager_new ();

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

  priv->battery_device = hal_device_new ();
  hal_device_set_udi (priv->battery_device, priv->battery_udi);
  hal_device_watch_property_modified (priv->battery_device);
  g_signal_connect (priv->battery_device,
                    "property-modified",
                    (GCallback)_hal_battery_device_property_modified,
                    self);
}

gint
dalston_battery_monitor_get_time_remaining (DalstonBatteryMonitor *monitor)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (monitor);
  gint value = -1;
  GError *error = NULL;

  if (!hal_device_get_int (priv->battery_device,
                           "battery.remaining_time",
                           &value,
                           &error))
  {
    g_warning (G_STRLOC ": Error getting time remaining: %s",
               error->message);
    g_clear_error (&error);
  }

  return value;
}

gint
dalston_battery_monitor_get_charge_percentage (DalstonBatteryMonitor *monitor)
{
  DalstonBatteryMonitorPrivate *priv = GET_PRIVATE (monitor);
  gint value = -1;
  GError *error = NULL;

  if (!hal_device_get_int (priv->battery_device,
                           "battery.charge_level.percentage",
                           &value,
                           &error))
  {
    g_warning (G_STRLOC ": Error getting charge percentage: %s",
               error->message);
    g_clear_error (&error);
  }

  return value;
}



