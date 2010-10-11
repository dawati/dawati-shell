/*
 * Carrick - a connection panel for the MeeGo Netbook
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Written by - Ross Burton <ross@linux.intel.com>
 *
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "carrick-util.h"

const char *
util_get_service_type_for_display (const char *type)
{
  /* TRANSLATORS: Service type names
   * These names will be used in notification-manager to form phrases 
   * such as
   *   "Sorry, your %s connection was lost. So we've connected
   *     you to a %s network"
   * where both placeholders are service type names. */
  if (g_strcmp0 (type, "wifi") == 0)
    return _("WiFi");
  else if (g_strcmp0 (type, "ethernet") == 0)
    return _("wired");
  else if (g_strcmp0 (type, "cellular") == 0)
    return _("3G");
  else if (g_strcmp0 (type, "wimax") == 0)
    return _("WiMAX");
  else if (g_strcmp0 (type, "bluetooth") == 0)
    return _("Bluetooth");
  else if (g_strcmp0 (type, "vpn") == 0)
    return _("VPN");
  else
    /* Run it through gettext and hope for the best */
    return _(type);
}


/* Return TRUE if passphrase seems to be good
 * 
 * In case of invalid passphrase, if msg is set, insert a 
 * infromational message about the problem in msg. This
 * string should be freed by caller.
 */
gboolean
util_validate_wlan_passphrase (const char *security,
                               const char *passphrase,
                               char **msg)
{
  guint len;
  gboolean valid = TRUE;
  
  len = strlen (passphrase);

  if (g_strcmp0 (security, "wep") == 0)
    {
      /*
       * 48/64-bit WEP keys are 5 or 10 characters (ASCII/hexadecimal)
       * 128-bit WEP keys are 13 or 26.
       * (Unofficial) 256-bit WEP keys are 29 or 58
       */
      if (len != 5 && len != 10 &&
          len != 13 && len != 26 &&
          len != 29 && len != 58)
        {
          valid = FALSE;
          if (msg)
            *msg = g_strdup_printf (_("Your password isn't the right length."
                                      " For a WEP connection it needs to be"
                                      " either 10 or 26 characters, you"
                                      " have %i."), len);
        }
    }
  else if (g_strcmp0 (security, "wpa") == 0 ||
           g_strcmp0 (security, "psk") == 0)
    {
      /* WPA passphrase must be between 8 and 63 characters (inclusive) */
      /* TODO: 64-character hex string, or 8-63 character ASCII */
      if (len < 8)
        {
          valid = FALSE;
          if (msg)
            *msg = g_strdup_printf (_("Your password is too short. For a WPA "
                                      " connection it needs to be at least"
                                      " 8 characters long, you have %i"), len);
        }
      else if (len > 64)
        {
          valid = FALSE;
          if (msg)
            *msg = g_strdup_printf (_("Your password is too long. For a WPA "
                                      " connection it needs to have fewer than"
                                      " 64 characters, you have %i"), len);
        }
    }
  else if (g_strcmp0 (security, "rsn") == 0)
    {
      /* WPA2 passphrase must be between 8 and 63 characters (inclusive) */
      /* TODO: 64-character hex string, or 8-63 character ASCII */
      if (len < 8)
        {
          valid = FALSE;
          if (msg)
            *msg = g_strdup_printf (_("Your password is too short. For a WPA2 "
                                      " connection it needs to be at least"
                                      " 8 characters long, you have %i"), len);
        }
      else if (len > 64)
        {
          valid = FALSE;
          if (msg)
            *msg = g_strdup_printf (_("Your password is too long. For a WPA2 "
                                      " connection it needs to have fewer than"
                                      " 64 characters, you have %i"), len);
        }
    }
  else if (g_strcmp0 (security, "none") == 0)
    ;
  else
    g_debug ("Don't know how to validate '%s' passphrase", security);

  return valid;
}
