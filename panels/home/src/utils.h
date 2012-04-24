/*
 * Copyright (c) 2012 Intel Corp.
 *
 * Author: Danielle Madeley <danielle.madeley@collabora.co.uk>
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

#include <glib.h>

#ifndef __UTILS_H__
#define __UTILS_H__

#define DEBUG(format, ...)  \
  g_debug ("%s: " format, G_STRFUNC, ##__VA_ARGS__)
#define STR_EMPTY(s) ((s) == NULL || *(s) == '\0')

G_BEGIN_DECLS

gboolean dconf_recursive_unset (const char *dir, GError **error);

G_END_DECLS

#endif
