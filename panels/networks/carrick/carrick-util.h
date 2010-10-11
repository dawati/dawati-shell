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

#ifndef _CARRICK_UTIL_H
#define _CARRICK_UTIL_H

#include <glib.h>

#define CARRICK_MIN_BUTTON_WIDTH 120

G_BEGIN_DECLS

const char * util_get_service_type_for_display (const char *type);

gboolean util_validate_wlan_passphrase (const char *security,
                                        const char *passphrase,
                                        char **msg);

G_END_DECLS

#endif /* _CARRICK_UTIL_H */
