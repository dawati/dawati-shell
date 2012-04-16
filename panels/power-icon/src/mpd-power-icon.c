
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
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
 */

#include <stdbool.h>

#include <glib/gi18n.h>
#include <gdk/gdkx.h>
#include <libnotify/notify.h>
#include <dawati-panel/mpl-panel-windowless.h>
#include <mx/mx.h>
#include <X11/XF86keysym.h>

#include "mpd-battery-device.h"
#include "mpd-gobject.h"
#include "mpd-global-key.h"
#include "mpd-power-icon.h"
#include "mpd-shutdown-notification.h"
#include "config.h"

G_DEFINE_TYPE (MpdPowerIcon, mpd_power_icon, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_POWER_ICON, MpdPowerIconPrivate))

typedef struct
{
  MplPanelClient      *panel;
  MpdBatteryDevice    *battery;
  MxAction            *shutdown_key;
  NotifyNotification  *shutdown_note;
  int                  last_notification_displayed;
  bool                 in_shutdown;
} MpdPowerIconPrivate;

typedef enum
{
  NOTIFICATION_NONE = 0,
  NOTIFICATION_20_PERCENT,
  NOTIFICATION_10_PERCENT,
  NOTIFICATION_5_PERCENT
} NotificationLevel;

static const struct
{
  const gchar *title;
  const gchar *message;
  const gchar *icon;
} _messages[] = {
  { NULL, NULL, NULL },
  { N_("Running low on battery"),
    N_("I've noticed that your battery is running a bit low. " \
       "If you can, it would be a good idea to plug in and top up."),
    NULL },
  { N_("Getting close to empty"),
    N_("You're running quite low on battery. " \
       "It'd be a good idea to save all your work " \
       "and plug in as soon as you can."),
    NULL },
  { N_("Danger!"),
    N_("Sorry, your computer is about to run out of battery. " \
       "I'm going to have to go to sleep now. " \
       "Please save your work and hope to see you again soon."),
    NULL },
  { NULL, NULL, NULL }
};

static void
shutdown (MpdPowerIcon *self)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (self);
  GError        *error = NULL;
  GDBusConnection *connection;
  GVariant *variant;

  priv->in_shutdown = true;

  connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);

  if (error)
  {
    g_critical ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);

    return;
  }

  variant = g_dbus_connection_call_sync (connection,
                                         "org.freedesktop.login1",
                                         "/org/freedesktop/login1",
                                         "org.freedesktop.login1.Manager",
                                         "PowerOff",
                                         g_variant_new ("(b)", TRUE), NULL,
                                         G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                                         &error);

  if (error)
  {
    g_critical ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
  else
  {
    g_variant_unref (variant);
  }

  g_object_unref (connection);
}

/*
 * Only test_percentage > 0 will be considered,
 * otherwise system percentage will be used.
 */
static void
update (MpdPowerIcon *self,
        int           test_percentage)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (self);
  char const            *button_style = NULL;
  char                  *description = NULL;
  MpdBatteryDeviceState  state;
  int                    percentage;

  state = mpd_battery_device_get_state (priv->battery);
  description = mpd_battery_device_get_state_text (priv->battery);

  if (test_percentage < 0)
    percentage = mpd_battery_device_get_percentage (priv->battery);
  else
    percentage = test_percentage;

  switch (state)
  {
  case MPD_BATTERY_DEVICE_STATE_MISSING:
    button_style = "state-missing";
    break;
  case MPD_BATTERY_DEVICE_STATE_CHARGING:
    button_style = "state-plugged";
    break;
  case MPD_BATTERY_DEVICE_STATE_DISCHARGING:
    if (percentage < 0)
      button_style = "state-missing";
    else if (percentage < 10)
      button_style = "state-empty";
    else if (percentage < 30)
      button_style = "state-20";
    else if (percentage < 50)
      button_style = "state-40";
    else if (percentage < 70)
      button_style = "state-60";
    else if (percentage < 90)
      button_style = "state-80";
    else
      button_style = "state-full";
    break;
  case MPD_BATTERY_DEVICE_STATE_FULLY_CHARGED:
    button_style = "state-full";
    break;
  default:
    button_style = "state-missing";
  }

  mpl_panel_client_request_button_style (priv->panel, button_style);
  mpl_panel_client_request_tooltip (priv->panel, description);

  g_free (description);
}

static void
_battery_percentage_notify_cb (MpdBatteryDevice *battery,
                               GParamSpec       *pspec,
                               MpdPowerIcon     *self)
{
  update (self, -1);
}

static void
_battery_state_notify_cb (MpdBatteryDevice *battery,
                          GParamSpec       *pspec,
                          MpdPowerIcon     *self)
{
  update (self, -1);
}

static void
_shutdown_notification_shutdown_cb (NotifyNotification  *notification,
                                    MpdPowerIcon        *self)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (self);

  shutdown (self);

  g_object_unref (priv->shutdown_note);
  priv->shutdown_note = NULL;
}

static void
_shutdown_notification_closed_cb (NotifyNotification  *notification,
                                  MpdPowerIcon        *self)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (self);

  g_object_unref (priv->shutdown_note);
  priv->shutdown_note = NULL;
}

static void
_shutdown_key_activated_cb (MxAction      *action,
                            MpdPowerIcon  *self)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (self);

  if (priv->shutdown_note)
  {
    /* Power key pressed again with notification up already. */
    shutdown (self);
  } else {
    priv->shutdown_note = mpd_shutdown_notification_new ();

    g_signal_connect (priv->shutdown_note, "closed",
                      G_CALLBACK (_shutdown_notification_closed_cb), self);
    g_signal_connect (priv->shutdown_note, "shutdown",
                      G_CALLBACK (_shutdown_notification_shutdown_cb), self);

    mpd_shutdown_notification_run (
                      MPD_SHUTDOWN_NOTIFICATION (priv->shutdown_note));
  }
}

static void
_dispose (GObject *object)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (object);

  mpd_gobject_detach (object, (GObject **) &priv->battery);
  mpd_gobject_detach (object, (GObject **) &priv->shutdown_key);
  mpd_gobject_detach (object, (GObject **) &priv->shutdown_note);

  G_OBJECT_CLASS (mpd_power_icon_parent_class)->dispose (object);
}

static void
mpd_power_icon_class_init (MpdPowerIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdPowerIconPrivate));

  object_class->dispose = _dispose;
}

static void
mpd_power_icon_init (MpdPowerIcon *self)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (self);
  GdkDisplay *display;
  unsigned int key_code;

  /* Panel */
  priv->panel = mpl_panel_windowless_new (MPL_PANEL_POWER,
                                          _("battery"),
                                          THEMEDIR "/power-icon.css",
                                          "unknown",
                                          true);

  /* Battery */
  priv->battery = mpd_battery_device_new ();
  g_signal_connect (priv->battery, "notify::percentage",
                    G_CALLBACK (_battery_percentage_notify_cb), self);
  g_signal_connect (priv->battery, "notify::state",
                    G_CALLBACK (_battery_state_notify_cb), self);

  update (self, -1);

  display = gdk_display_get_default ();

  /* Shutdown key. */
  key_code = XKeysymToKeycode (GDK_DISPLAY_XDISPLAY (display), XF86XK_PowerOff);
  if (key_code)
  {
    priv->shutdown_key = mpd_global_key_new (key_code);
    g_object_ref_sink (priv->shutdown_key);
    g_signal_connect (priv->shutdown_key, "activated",
                      G_CALLBACK (_shutdown_key_activated_cb), self);
  } else {
    g_warning ("Failed to query XF86XK_PowerOff key code.");
  }
}

MpdPowerIcon *
mpd_power_icon_new (void)
{
  return g_object_new (MPD_TYPE_POWER_ICON, NULL);
}

void
mpd_power_icon_test_notification (MpdPowerIcon *self,
                                  unsigned int  percentage)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MPD_IS_POWER_ICON (self));

  priv->last_notification_displayed = NOTIFICATION_NONE;
  update (self, percentage);
}

