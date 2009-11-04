/*
 * Carrick - a connection panel for the Moblin Netbook
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
  else
    /* Run it through gettext and hope for the best */
    return _(type);
}
