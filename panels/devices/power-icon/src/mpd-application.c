
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

#include <X11/XF86keysym.h>
#include <glib/gi18n.h>
#include <egg-console-kit/egg-console-kit.h>
#include <gdk/gdkx.h>
#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-panel-windowless.h>
#include <mx/mx.h>
#include <libnotify/notify.h>
#include "mpd-battery-device.h"
#include "mpd-global-key.h"
#include "mpd-idle-manager.h"
#include "mpd-lid-device.h"
#include "mpd-shutdown-notification.h"
#include "mpd-application.h"

G_DEFINE_TYPE (MpdApplication, mpd_application, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_APPLICATION, MpdApplicationPrivate))

typedef struct
{
  MplPanelClient      *panel;
  MpdBatteryDevice    *battery;
  MpdLidDevice        *lid;
  MpdIdleManager      *idle_manager;
  MxAction            *shutdown_key;
  NotifyNotification  *battery_note;
  NotifyNotification  *shutdown_note;
  int                  last_notification_displayed;
} MpdApplicationPrivate;

typedef enum
{
  NOTIFICATION_NONE,
  NOTIFICATION_20_PERCENT,
  NOTIFICATION_10_PERCENT,
  NOTIFICATION_5_PERCENT,
  NOTIFICATION_LAST
} NotificationLevel;

static const struct
{
  const gchar *title;
  const gchar *message;
  const gchar *icon;
} _messages[] = {
  { N_("Running low on battery"),
    N_("We've noticed that your battery is running a bit low. " \
       "If you can it would be a good idea to plug in and top up."),
    NULL },
  { N_("Getting close to empty"),
    N_("You're running quite low on battery. " \
       "It'd be a good idea to save all your work " \
       "and plug in as soon as you can"),
    NULL },
  { N_("Danger!"),
    N_("Sorry, your computer is about to run out of battery. " \
       "We're going to have to turn off now. " \
       "Please save your work and hope to see you again soon."),
    NULL }
};

static void
_shutdown_timeout_cb (MpdApplication *self)
{
  MpdApplicationPrivate *priv = GET_PRIVATE (self);
  MpdBatteryDeviceState  state;
  GError                *error = NULL;

  state = mpd_battery_device_get_state (priv->battery);
  if (state == MPD_BATTERY_DEVICE_STATE_DISCHARGING)
  {
    EggConsoleKit *console = egg_console_kit_new ();
    egg_console_kit_stop (console, &error);
    if (error)
    {
      g_critical ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }
    g_object_unref (console);
  }
}

static void
do_notification (MpdApplication     *self,
                 NotificationLevel   level)
{
  MpdApplicationPrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;

  priv->battery_note = notify_notification_new (_(_messages[level].title),
                                                _(_messages[level].message),
                                                _messages[level].icon,
                                                NULL);

  notify_notification_set_timeout (priv->battery_note, 10000);

  if (level == NOTIFICATION_10_PERCENT)
  {
    notify_notification_set_urgency (priv->battery_note,
                                     NOTIFY_URGENCY_CRITICAL);
  }

  if (!notify_notification_show (priv->battery_note, &error))
  {
    g_warning (G_STRLOC ": Error showing notification: %s",
               error->message);
    g_clear_error (&error);
  }

  g_object_unref (priv->battery_note);
}

static void
update (MpdApplication *self)
{
  MpdApplicationPrivate *priv = GET_PRIVATE (self);
  char const            *button_style = NULL;
  char                  *description = NULL;
  MpdBatteryDeviceState  state;
  int                    percentage;

  state = mpd_battery_device_get_state (priv->battery);
  percentage = mpd_battery_device_get_percentage (priv->battery);

  switch (state)
  {
  case MPD_BATTERY_DEVICE_STATE_MISSING:
    button_style = "state-missing";
    description = g_strdup_printf (_("Sorry, you don't appear to have a "
                                     "battery installed."));
    break;
  case MPD_BATTERY_DEVICE_STATE_CHARGING:
    button_style = "state-plugged";
    description = g_strdup_printf (_("Your battery is charging. "
                                     "It is about %d%% full."),
                                   percentage);
    break;
  case MPD_BATTERY_DEVICE_STATE_DISCHARGING:
    description = g_strdup_printf (_("Your battery is being used. "
                                     "It is about %d%% full."),
                                   percentage);
    break;
  case MPD_BATTERY_DEVICE_STATE_FULLY_CHARGED:
    button_style = "state-full";
    description = g_strdup_printf (_("Your battery is fully charged and "
                                     "you're ready to go."));
    break;
  default:
    button_style = "state-missing";
    description = g_strdup_printf (_("Sorry, it looks like your battery is "
                                     "broken."));
  }

  if (!button_style)
  {
    if (percentage < 0)
      button_style = "state-missing";
    else if (percentage < 10)
      button_style = "state-empty";
    else if (percentage < 30)
      button_style = "state-25";
    else if (percentage < 60)
      button_style = "state-50";
    else if (percentage < 90)
      button_style = "state-75";
    else
      button_style = "state-full";
  }

  mpl_panel_client_request_button_style (priv->panel, button_style);
  mpl_panel_client_request_tooltip (priv->panel, description);

  g_debug ("%s '%s' %d", description, button_style, percentage);

  g_free (description);

  if (state == MPD_BATTERY_DEVICE_STATE_DISCHARGING)
  {
    /* Do notifications at various levels */
    if (percentage > 0 && percentage < 5) {
      if (priv->last_notification_displayed != NOTIFICATION_10_PERCENT)
      {
        do_notification (self, NOTIFICATION_10_PERCENT);
        priv->last_notification_displayed = NOTIFICATION_10_PERCENT;

        g_timeout_add_seconds (60,
                               (GSourceFunc) _shutdown_timeout_cb,
                               self);
      }
    } else if (percentage < 10) {
      if (priv->last_notification_displayed != NOTIFICATION_10_PERCENT)
      {
        do_notification (self, NOTIFICATION_10_PERCENT);
        priv->last_notification_displayed = NOTIFICATION_10_PERCENT;
      }
    } else if (percentage < 20) {
      if (priv->last_notification_displayed != NOTIFICATION_20_PERCENT)
      {
        do_notification (self, NOTIFICATION_20_PERCENT);
        priv->last_notification_displayed = NOTIFICATION_20_PERCENT;
      }
    } else {
      /* Reset the notification */
      priv->last_notification_displayed = NOTIFICATION_NONE;
    }
  }
}

static void
_device_notify_cb (MpdBatteryDevice *battery,
                   GParamSpec       *pspec,
                   MpdApplication   *self)
{
  update (self);
}

static void
_lid_closed_cb (MpdLidDevice    *lid,
                GParamSpec      *pspec,
                MpdApplication  *self)
{
  MpdApplicationPrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;

  g_debug ("%s() %d", __FUNCTION__, mpd_lid_device_get_closed (lid));

  if (mpd_lid_device_get_closed (lid))
  {
    mpd_idle_manager_suspend (priv->idle_manager, &error);
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }
  }
}

static void
_shutdown_notification_shutdown_cb (NotifyNotification  *notification,
                                    MpdApplication      *self)
{
  EggConsoleKit *console;
  GError        *error = NULL;

  console = egg_console_kit_new ();
  egg_console_kit_stop (console, &error);
  if (error)
  {
    g_critical ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  g_object_unref (console);
  g_object_unref (notification);
}

static void
_shutdown_notification_closed_cb (NotifyNotification  *notification,
                                  MpdApplication      *self)
{
  g_debug ("%s()", __FUNCTION__);

  g_object_unref (notification);
}

static void
_shutdown_key_activated_cb (MxAction        *action,
                            MpdApplication  *self)
{
  MpdApplicationPrivate *priv = GET_PRIVATE (self);

  priv->shutdown_note = mpd_shutdown_notification_new (
                        _("Would you like to turn off now?"),
                        _("If you don't decide I'll turn off in 30 seconds."));

  g_signal_connect (priv->shutdown_note, "closed",
                    G_CALLBACK (_shutdown_notification_closed_cb), self);
  g_signal_connect (priv->shutdown_note, "shutdown",
                    G_CALLBACK (_shutdown_notification_shutdown_cb), self);

  mpd_shutdown_notification_run (MPD_SHUTDOWN_NOTIFICATION (priv->shutdown_note));
}

static void
_dispose (GObject *object)
{
  MpdApplicationPrivate *priv = GET_PRIVATE (object);

  if (priv->idle_manager)
  {
    g_object_unref (priv->idle_manager);
    priv->idle_manager = NULL;
  }

  if (priv->battery)
  {
    g_object_unref (priv->battery);
    priv->battery = NULL;
  }

  if (priv->lid)
  {
    g_object_unref (priv->lid);
    priv->lid = NULL;
  }

  if (priv->shutdown_key)
  {
    g_object_unref (priv->shutdown_key);
    priv->shutdown_key = NULL;
  }

  G_OBJECT_CLASS (mpd_application_parent_class)->dispose (object);
}

static void
mpd_application_class_init (MpdApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdApplicationPrivate));

  object_class->dispose = _dispose;
}

static void
mpd_application_init (MpdApplication *self)
{
  MpdApplicationPrivate *priv = GET_PRIVATE (self);
  unsigned int shutdown_key_code;

  /* Panel */
  priv->panel = mpl_panel_windowless_new (MPL_PANEL_POWER,
                                          _("battery"),
                                          PKGTHEMEDIR "/power-icon.css",
                                          "unknown",
                                          true);

  /* Battery */
  priv->battery = mpd_battery_device_new ();
  g_signal_connect (priv->battery, "notify::percentage",
                    G_CALLBACK (_device_notify_cb), self);
  g_signal_connect (priv->battery, "notify::state",
                    G_CALLBACK (_device_notify_cb), self);

  update (self);

  /* Idle manager */
  priv->idle_manager = mpd_idle_manager_new ();

  /* Shutdown key. */
  shutdown_key_code = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_PowerOff);
  if (shutdown_key_code)
  {
    priv->shutdown_key = mpd_global_key_new (shutdown_key_code);
    g_object_ref_sink (priv->shutdown_key);
    g_signal_connect (priv->shutdown_key, "activated",
                      G_CALLBACK (_shutdown_key_activated_cb), self);
  } else {
    g_warning ("Failed to query XF86XK_PowerOff key code.");
  }

  /* Lid. */
  priv->lid = mpd_lid_device_new ();
  g_signal_connect (priv->lid, "notify::closed",
                    G_CALLBACK (_lid_closed_cb), self);
}

MpdApplication *
mpd_application_new (void)
{
  return g_object_new (MPD_TYPE_APPLICATION, NULL);
}

