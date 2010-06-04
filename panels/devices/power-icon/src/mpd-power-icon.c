
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

#include <egg-console-kit/egg-console-kit.h>
#include <glib/gi18n.h>
#include <gdk/gdkx.h>
#include <libnotify/notify.h>
#include <moblin-panel/mpl-panel-windowless.h>
#include <mx/mx.h>
#include <X11/XF86keysym.h>

#include "mpd-battery-device.h"
#include "mpd-display-device.h"
#include "mpd-gobject.h"
#include "mpd-global-key.h"
#include "mpd-idle-manager.h"
#include "mpd-lid-device.h"
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
  MpdDisplayDevice    *display;
  MpdLidDevice        *lid;
  MpdIdleManager      *idle_manager;
  MxAction            *shutdown_key;
  MxAction            *sleep_key;
  MxAction            *brightness_up_key;
  MxAction            *brightness_down_key;
  NotifyNotification  *shutdown_note;
  int                  last_notification_displayed;
  unsigned int         shutdown_timeout_id;
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
       "I'm going to have to turn off now. " \
       "Please save your work and hope to see you again soon."),
    NULL },
  { NULL, NULL, NULL }
};

static void
shutdown (MpdPowerIcon *self)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (self);
  EggConsoleKit *console;
  GError        *error = NULL;

  priv->in_shutdown = true;

  console = egg_console_kit_new ();
  egg_console_kit_stop (console, &error);
  if (error)
  {
    g_critical ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
  g_object_unref (console);
}

static void
_battery_shutdown_timeout_cb (MpdPowerIcon *self)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (self);
  MpdBatteryDeviceState  state;

  priv->shutdown_timeout_id = 0;

  state = mpd_battery_device_get_state (priv->battery);
  if (state == MPD_BATTERY_DEVICE_STATE_DISCHARGING)
  {
    shutdown (self);
  }
}

static void
_notification_closed_cb (NotifyNotification *note,
                         MpdPowerIcon       *self)
{
  g_object_unref (note);
}

static void
do_notification (MpdPowerIcon       *self,
                 NotificationLevel   level,
                 NotifyUrgency       urgency)
{
  NotifyNotification  *note;
  GError              *error = NULL;

  g_return_if_fail (level > NOTIFICATION_NONE);

  note = notify_notification_new (_(_messages[level].title),
                                  _(_messages[level].message),
                                  _messages[level].icon,
                                  NULL);
  notify_notification_set_urgency (note, urgency);
  g_signal_connect (note, "closed",
                    G_CALLBACK (_notification_closed_cb), self);

  notify_notification_show (note, &error);
  if (error)
  {
    g_warning ("%s : Error showing notification: %s",
               G_STRLOC,
               error->message);
    g_clear_error (&error);
  }
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

  if (state == MPD_BATTERY_DEVICE_STATE_DISCHARGING)
  {
    /* Do notifications at various levels */
    if (percentage > 0 && percentage < 5 &&
        priv->last_notification_displayed != NOTIFICATION_5_PERCENT)
    {
      do_notification (self,
                        NOTIFICATION_5_PERCENT,
                        NOTIFY_URGENCY_CRITICAL);
      priv->last_notification_displayed = NOTIFICATION_5_PERCENT;

      if (0 == priv->shutdown_timeout_id)
      {
        priv->shutdown_timeout_id = g_timeout_add_seconds (
                                60,
                                (GSourceFunc) _battery_shutdown_timeout_cb,
                                self);
      }
    } else if (percentage < 10 &&
               priv->last_notification_displayed != NOTIFICATION_10_PERCENT)
    {
      do_notification (self,
                        NOTIFICATION_10_PERCENT,
                        NOTIFY_URGENCY_CRITICAL);
      priv->last_notification_displayed = NOTIFICATION_10_PERCENT;

    } else if (percentage < 20 &&
               priv->last_notification_displayed != NOTIFICATION_20_PERCENT)
    {
      do_notification (self,
                        NOTIFICATION_20_PERCENT,
                        NOTIFY_URGENCY_NORMAL);
      priv->last_notification_displayed = NOTIFICATION_20_PERCENT;

    } else
    {
      /* Reset the notification */
      priv->last_notification_displayed = NOTIFICATION_NONE;
    }
  } else
  {
    /* Not discharging, reset so we start over correctly when
     * plugged out again immediately. */

    if (priv->shutdown_timeout_id)
    {
      g_source_remove (priv->shutdown_timeout_id);
      priv->shutdown_timeout_id = 0;
    }

    priv->last_notification_displayed = NOTIFICATION_NONE;
  }
}

static void
_device_notify_cb (MpdBatteryDevice *battery,
                   GParamSpec       *pspec,
                   MpdPowerIcon     *self)
{
  update (self, -1);
}

static void
_lid_closed_cb (MpdLidDevice    *lid,
                GParamSpec      *pspec,
                MpdPowerIcon    *self)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;

  if (mpd_lid_device_get_closed (lid) &&
      !priv->in_shutdown)
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
    priv->shutdown_note = mpd_shutdown_notification_new (
                        _("Would you like to turn off now?"),
                        _("If you don't decide, I'll turn off in 30 seconds."));

    g_signal_connect (priv->shutdown_note, "closed",
                      G_CALLBACK (_shutdown_notification_closed_cb), self);
    g_signal_connect (priv->shutdown_note, "shutdown",
                      G_CALLBACK (_shutdown_notification_shutdown_cb), self);

    mpd_shutdown_notification_run (
                      MPD_SHUTDOWN_NOTIFICATION (priv->shutdown_note));
  }
}

static void
_sleep_key_activated_cb (MxAction     *action,
                         MpdPowerIcon *self)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (self);
  GError  *error = NULL;

  mpd_idle_manager_suspend (priv->idle_manager, &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
}

static void
_brightness_up_key_activated_cb (MxAction     *action,
                                 MpdPowerIcon *self)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (self);
  float brightness;

  g_debug ("%s()", __FUNCTION__);

  brightness = mpd_display_device_get_brightness (priv->display);
  if (brightness >= 0)
    mpd_display_device_set_brightness (priv->display, brightness + 0.1);
  else
    g_warning ("%s : Brightness is %.1f", G_STRLOC, brightness);
}

static void
_brightness_down_key_activated_cb (MxAction     *action,
                                   MpdPowerIcon *self)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (self);
  float brightness;

  g_debug ("%s()", __FUNCTION__);

  brightness = mpd_display_device_get_brightness (priv->display);
  if (brightness >= 0)
    mpd_display_device_set_brightness (priv->display, brightness - 0.1);
  else
    g_warning ("%s : Brightness is %.1f", G_STRLOC, brightness);
}

static void
_dispose (GObject *object)
{
  MpdPowerIconPrivate *priv = GET_PRIVATE (object);

  mpd_gobject_detach (object, (GObject **) &priv->idle_manager);

  mpd_gobject_detach (object, (GObject **) &priv->battery);

  /* There's some bug in GpmBrightnessXRandR (not freeing the filter?)
   * so we're leaking this here.
   * mpd_gobject_detach (object, (GObject **) &priv->display); */

  mpd_gobject_detach (object, (GObject **) &priv->lid);

  mpd_gobject_detach (object, (GObject **) &priv->shutdown_key);

  mpd_gobject_detach (object, (GObject **) &priv->sleep_key);

  mpd_gobject_detach (object, (GObject **) &priv->brightness_up_key);
  mpd_gobject_detach (object, (GObject **) &priv->brightness_down_key);

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
  unsigned int key_code;

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

  update (self, -1);

  /* Idle manager */
  priv->idle_manager = mpd_idle_manager_new ();

  /* Shutdown key. */
  key_code = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_PowerOff);
  if (key_code)
  {
    priv->shutdown_key = mpd_global_key_new (key_code);
    g_object_ref_sink (priv->shutdown_key);
    g_signal_connect (priv->shutdown_key, "activated",
                      G_CALLBACK (_shutdown_key_activated_cb), self);
  } else {
    g_warning ("Failed to query XF86XK_PowerOff key code.");
  }

  /* Sleep key. */
  key_code = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_Sleep);
  if (key_code)
  {
    priv->sleep_key = mpd_global_key_new (key_code);
    g_object_ref_sink (priv->sleep_key);
    g_signal_connect (priv->sleep_key, "activated",
                      G_CALLBACK (_sleep_key_activated_cb), self);
  } else {
    g_warning ("Failed to query XF86XK_PowerOff key code.");
  }

  /* Display */
  priv->display = mpd_display_device_new ();
  if (mpd_display_device_is_enabled (priv->display))
  {
    /* Brightness keys. */
    key_code = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_MonBrightnessUp);
    if (key_code)
    {
      priv->brightness_up_key = mpd_global_key_new (key_code);
      g_object_ref_sink (priv->brightness_up_key);
      g_signal_connect (priv->brightness_up_key, "activated",
                        G_CALLBACK (_brightness_up_key_activated_cb), self);
    } else {
      g_warning ("Failed to query XF86XK_MonBrightnessUp key code.");
    }
    key_code = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_MonBrightnessDown);
    if (key_code)
    {
      priv->brightness_down_key = mpd_global_key_new (key_code);
      g_object_ref_sink (priv->brightness_down_key);
      g_signal_connect (priv->brightness_down_key, "activated",
                        G_CALLBACK (_brightness_down_key_activated_cb), self);
    } else {
      g_warning ("Failed to query XF86XK_MonBrightnessDown key code.");
    }
  }

  /* Lid. */
  priv->lid = mpd_lid_device_new ();
  g_signal_connect (priv->lid, "notify::closed",
                    G_CALLBACK (_lid_closed_cb), self);
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

