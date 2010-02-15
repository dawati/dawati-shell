
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Bradford <rob@linux.intel.com> (dalston-power-applet.c)
 *          Rob Staudinger <robert.staudinger@intel.com>
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
#include <locale.h>
#include <stdlib.h>
#include <X11/XF86keysym.h>
#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
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
#include "config.h"

typedef enum
{
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

static int _last_notification_displayed = -1;

static void
_shutdown_timeout_cb (MpdBatteryDevice *battery)
{
  MpdBatteryDeviceState  state;
  GError                *error = NULL;

  state = mpd_battery_device_get_state (battery);
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
do_notification (NotificationLevel level)
{
  NotifyNotification *note;
  GError *error = NULL;

  note = notify_notification_new (_(_messages[level].title),
                                  _(_messages[level].message),
                                  _messages[level].icon,
                                  NULL);

  notify_notification_set_timeout (note, 10000);

  if (level == NOTIFICATION_10_PERCENT)
  {
    notify_notification_set_urgency (note, NOTIFY_URGENCY_CRITICAL);
  }

  if (!notify_notification_show (note, &error))
  {
    g_warning (G_STRLOC ": Error showing notification: %s",
               error->message);
    g_clear_error (&error);
  }

  g_object_unref (note);
}

static void
update (MpdBatteryDevice  *battery,
        MplPanelClient    *client)
{
  char const            *button_style = NULL;
  char                  *description = NULL;
  MpdBatteryDeviceState  state;
  int                    percentage;

  state = mpd_battery_device_get_state (battery);
  percentage = mpd_battery_device_get_percentage (battery);

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

  mpl_panel_client_request_button_style (client, button_style);
  mpl_panel_client_request_tooltip (client, description);

  g_debug ("%s '%s' %d", description, button_style, percentage);

  g_free (description);

  if (state == MPD_BATTERY_DEVICE_STATE_DISCHARGING)
  {
    /* Do notifications at various levels */
    if (percentage > 0 && percentage < 5) {
      if (_last_notification_displayed != NOTIFICATION_10_PERCENT)
      {
        do_notification (NOTIFICATION_10_PERCENT);
        _last_notification_displayed = NOTIFICATION_10_PERCENT;

        g_timeout_add_seconds (60,
                               (GSourceFunc) _shutdown_timeout_cb,
                               battery);
      }
    } else if (percentage < 10) {
      if (_last_notification_displayed != NOTIFICATION_10_PERCENT)
      {
        do_notification (NOTIFICATION_10_PERCENT);
        _last_notification_displayed = NOTIFICATION_10_PERCENT;
      }
    } else if (percentage < 20) {
      if (_last_notification_displayed != NOTIFICATION_20_PERCENT)
      {
        do_notification (NOTIFICATION_20_PERCENT);
        _last_notification_displayed = NOTIFICATION_20_PERCENT;
      }
    } else {
      /* Reset the notification */
      _last_notification_displayed = -1;
    }
  }
}

static void
_device_notify_cb (MpdBatteryDevice *battery,
                   GParamSpec       *pspec,
                   MplPanelClient   *client)
{
  update (battery, client);
}

static void
_lid_closed_cb (MpdLidDevice    *lid,
                GParamSpec      *pspec,
                MpdIdleManager  *idlr)
{
  GError *error = NULL;

  if (mpd_lid_device_get_closed (lid))
  {
    mpd_idle_manager_suspend (idlr, &error);
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }
  }
}

static void
_shutdown_notification_shutdown_cb (NotifyNotification *notification,
                                    gpointer            userdata)
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
_shutdown_notification_closed_cb (NotifyNotification *notification,
                                  gpointer            userdata)
{
  g_debug ("%s()", __FUNCTION__);

  g_object_unref (notification);
}

static void
_shutdown_key_activated_cb (MxAction  *action,
                            gpointer   data)
{
  NotifyNotification *notification;

  notification = mpd_shutdown_notification_new (
                        _("Would you like to turn off now?"),
                        _("If you don't decide I'll turn off in 30 seconds."));

  g_signal_connect (notification, "closed",
                    G_CALLBACK (_shutdown_notification_closed_cb), NULL);
  g_signal_connect (notification, "shutdown",
                    G_CALLBACK (_shutdown_notification_shutdown_cb), NULL);

  mpd_shutdown_notification_run (MPD_SHUTDOWN_NOTIFICATION (notification));
}

static MxAction *
create_shutdown_key (void)
{
  MxAction  *shutdown_key = NULL;
  guint      shutdown_key_code;

  shutdown_key_code = XKeysymToKeycode (GDK_DISPLAY (), XF86XK_PowerOff);
  if (shutdown_key_code)
  {
    shutdown_key = mpd_global_key_new (shutdown_key_code);
    g_signal_connect (shutdown_key, "activated",
                      G_CALLBACK (_shutdown_key_activated_cb), NULL);
  } else {
    g_warning ("Failed to query XF86XK_PowerOff key code.");
  }

  return shutdown_key;
}

int
main (int    argc,
      char **argv)
{
  MplPanelClient    *client;
  MpdBatteryDevice  *battery;
  MpdLidDevice      *lid;
  MpdIdleManager    *idle_manager;
  MxAction          *shutdown_key;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_clutter_init (&argc, &argv);
  notify_init ("Moblin Power Icon");

  client = mpl_panel_windowless_new (MPL_PANEL_POWER,
                                     _("battery"),
                                     PKGTHEMEDIR "/power-icon.css",
                                     "unknown",
                                     true);

  battery = mpd_battery_device_new ();
  g_signal_connect (battery, "notify::percentage",
                    G_CALLBACK (_device_notify_cb), client);
  g_signal_connect (battery, "notify::state",
                    G_CALLBACK (_device_notify_cb), client);

  update (battery, client);

  idle_manager = mpd_idle_manager_new ();

  /* Hook up shutdown key. */
  shutdown_key = create_shutdown_key ();
  g_object_ref_sink (shutdown_key);

  /* Hook up lid. */
  lid = mpd_lid_device_new ();
  g_object_ref_sink (lid);
  g_signal_connect (lid, "notify::closed",
                    G_CALLBACK (_lid_closed_cb), idle_manager);

  clutter_main ();

  g_object_unref (idle_manager);
  g_object_unref (battery);
  g_object_unref (lid);
  g_object_unref (shutdown_key);

  return EXIT_SUCCESS;
}

