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
#include <libnotify/notify.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (DalstonButtonMonitor, dalston_button_monitor, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_BUTTON_MONITOR, DalstonButtonMonitorPrivate))

typedef struct _DalstonButtonMonitorPrivate DalstonButtonMonitorPrivate;

struct _DalstonButtonMonitorPrivate {
  HalPowerProxy *power_proxy;
  HalManager *manager;
  GList *devices;
  gboolean is_hal_running;

  NotifyNotification *shutdown_notification;
  guint shutdown_notification_timeout;
  gint shutdown_seconds_remaining;
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
_cleanup_button_devices (DalstonButtonMonitor *self)
{
  DalstonButtonMonitorPrivate *priv = GET_PRIVATE (self);
  GList *l = NULL;
  HalDevice *device = NULL;

  for (l = priv->devices; l; l = g_list_delete_link (l, l))
  {
    device = (HalDevice *)l->data;

    if (device)
    {
      g_object_unref (device);
    }
  }

  priv->devices = l;
}

static void
dalston_button_monitor_dispose (GObject *object)
{
  DalstonButtonMonitorPrivate *priv = GET_PRIVATE (object);

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

  /* yes, we are sure it's a DalstonButtonMonitor */
  _cleanup_button_devices ((DalstonButtonMonitor *)object);

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
_shutdown_notify_cb (NotifyNotification *notification,
                     gchar              *action,
                     gpointer            userdata)
{
  DalstonButtonMonitor *monitor = (DalstonButtonMonitor *)userdata;
  DalstonButtonMonitorPrivate *priv = GET_PRIVATE (monitor);

  hal_power_proxy_shutdown_sync (priv->power_proxy);
}

static void
_shutdown_notify_closed_cb (NotifyNotification *notification,
                            gpointer            userdata)
{
  DalstonButtonMonitor *monitor = (DalstonButtonMonitor *)userdata;
  DalstonButtonMonitorPrivate *priv = GET_PRIVATE (monitor);

  if (priv->shutdown_notification_timeout > 0)
  {
    g_source_remove (priv->shutdown_notification_timeout);
    priv->shutdown_notification_timeout = 0;
  }

  g_object_unref (priv->shutdown_notification);
  priv->shutdown_notification = NULL;
}

static gboolean
_shutdown_notify_timeout_cb (gpointer userdata)
{
  DalstonButtonMonitor *monitor = (DalstonButtonMonitor *)userdata;
  DalstonButtonMonitorPrivate *priv = GET_PRIVATE (monitor);
  gchar *summary_text;

  priv->shutdown_seconds_remaining -= 5;

  if (priv->shutdown_seconds_remaining == 0)
  {
    notify_notification_close (priv->shutdown_notification, NULL);
    hal_power_proxy_shutdown_sync (priv->power_proxy);
    return FALSE;
  }

  summary_text = g_strdup_printf (_("If you don't decide i'll turn off in %d seconds"),
                                  priv->shutdown_seconds_remaining);
  g_object_set (priv->shutdown_notification,
                "summary",
                summary_text,
                NULL);

  g_free (summary_text);

  return TRUE;
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
  gboolean state = FALSE;
  gboolean has_state = FALSE;

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
    g_debug (G_STRLOC ": Got lid button signal");

    hal_device_get_bool (device,
                         "button.has_state",
                         &has_state,
                         &error);

    if (error)
    {
      g_warning (G_STRLOC ": Error getting if button has state: %s",
                 error->message);
      g_clear_error (&error);
    } else {
      if (has_state)
      {
        hal_device_get_bool (device,
                             "button.state.value",
                             &state,
                             &error);

        if (error)
        {
          g_warning (G_STRLOC ": Error getting button state: %s",
                     error->message);
          g_clear_error (&error);
        } else {
          g_debug (G_STRLOC ": Lid button has state: %s",
                   state ? "on" : "off");

          /* Shutdown notification inhibits suspend */
          if (state && !priv->shutdown_notification)
            hal_power_proxy_suspend_sync (priv->power_proxy);
        }
      } else {
        /* Shutdown notification inhibits suspend */
        if (!priv->shutdown_notification)
          hal_power_proxy_suspend_sync (priv->power_proxy);
      }
    }
  } else if (g_str_equal (type, "power")) {

    if (priv->shutdown_notification)
    {
      hal_power_proxy_shutdown_sync (priv->power_proxy);
      g_free (type);
      return;
    }

    priv->shutdown_notification = notify_notification_new (_("Would you like to turn off now?"),
                                                           _("If you don't decide i'll turn off in 30 seconds"),
                                                           NULL,
                                                           NULL);
    notify_notification_set_urgency (priv->shutdown_notification, NOTIFY_URGENCY_CRITICAL);
    notify_notification_set_timeout (priv->shutdown_notification, NOTIFY_EXPIRES_NEVER);
    notify_notification_add_action (priv->shutdown_notification,
                                    "shutdown",
                                    _("Turn off"),
                                    _shutdown_notify_cb,
                                    g_object_ref (monitor),
                                    (GFreeFunc)g_object_unref);
    g_signal_connect (priv->shutdown_notification,
                      "closed",
                      (GCallback)_shutdown_notify_closed_cb,
                      monitor);

    priv->shutdown_seconds_remaining = 30;
    priv->shutdown_notification_timeout = g_timeout_add_seconds (5,
                                                                 _shutdown_notify_timeout_cb,
                                                                 monitor);

    if (!notify_notification_show (priv->shutdown_notification,
                                   &error))
    {
      g_warning (G_STRLOC ": Error showing notification: %s",
                 error->message);
      g_clear_error (&error);
    }
  }

  g_free (type);
}

static void
_setup_button_devices (DalstonButtonMonitor *self)
{
  DalstonButtonMonitorPrivate *priv = GET_PRIVATE (self);
  gchar **names;
  GError *error = NULL;
  gchar *udi;
  gint i;
  HalDevice *device;
  gchar *type;

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

static void
_hal_daemon_start (HalManager           *manager,
                   DalstonButtonMonitor *self)
{
  DalstonButtonMonitorPrivate *priv = GET_PRIVATE (self);

  /* spurious signal ? */
  if (priv->is_hal_running)
    return;

  _setup_button_devices (self);

  priv->is_hal_running = TRUE;
}

static void
_hal_daemon_stop (HalManager           *manager,
                  DalstonButtonMonitor *self)
{
  DalstonButtonMonitorPrivate *priv = GET_PRIVATE (self);

  /* spurious signal ? */
  if (!priv->is_hal_running)
    return;

  _cleanup_button_devices (self);

  priv->is_hal_running = FALSE;
}

static void
dalston_button_monitor_init (DalstonButtonMonitor *self)
{
  DalstonButtonMonitorPrivate *priv = GET_PRIVATE (self);

  priv->power_proxy = hal_power_proxy_new ();

  if (!priv->power_proxy)
  {
    g_warning (G_STRLOC ": No power proxy; not listening for buttons.");
    return;
  }

  priv->manager = hal_manager_new ();
  g_signal_connect (priv->manager,
                    "daemon-start",
                    G_CALLBACK (_hal_daemon_start),
                    self);
  g_signal_connect (priv->manager,
                    "daemon-stop",
                    G_CALLBACK (_hal_daemon_stop),
                    self);

  priv->is_hal_running = FALSE;
  if (hal_manager_is_running (priv->manager))
  {
    priv->is_hal_running = TRUE;
    _setup_button_devices (self);
  }
}

DalstonButtonMonitor *
dalston_button_monitor_new (void)
{
  return g_object_new (DALSTON_TYPE_BUTTON_MONITOR, NULL);
}


