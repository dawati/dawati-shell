
/*
 * Copyright (C) 2009-2010, Intel Corporation.
 *
 * Author: Robert Staudinger <robert.staudinger@intel.com>
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

#include <stdlib.h>
#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-panel-windowless.h>
#include "mpd-battery-device.h"
#include "config.h"

static void
update (MpdBatteryDevice  *battery,
        MplPanelClient    *client)
{
  gchar const           *button_style = NULL;
  gchar                 *description = NULL;
  MpdBatteryDeviceState  state;
  gint                   percentage;

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
}

static void
_device_notify_cb (MpdBatteryDevice *battery,
                   GParamSpec       *pspec,
                   MplPanelClient   *client)
{
  update (battery, client);
}

int
main (int    argc,
      char **argv)
{
  MplPanelClient    *client;
  MpdBatteryDevice  *battery;

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  clutter_init (&argc, &argv);

  client = mpl_panel_windowless_new (MPL_PANEL_POWER,
                                     _("battery"),
                                     PKGTHEMEDIR "/toolbar-button.css",
                                     "unknown",
                                     TRUE);

  battery = mpd_battery_device_new ();
  g_signal_connect (battery, "notify::percentage",
                    G_CALLBACK (_device_notify_cb), client);
  g_signal_connect (battery, "notify::state",
                    G_CALLBACK (_device_notify_cb), client);

  clutter_main ();
  g_object_unref (battery);
  return EXIT_SUCCESS;
}

