/*
 * Copyright (c) 2011 Intel Corp.
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

#include <dconf-client.h>
#include <dconf-paths.h>

#include "utils.h"

gboolean
dconf_recursive_unset (const char *dir,
    GError **error)
{
  DConfClient *client = dconf_client_new (NULL, NULL, NULL, NULL);
  gboolean r;

  g_return_val_if_fail (dconf_is_dir (dir, error), FALSE);

  r = dconf_client_write (client, dir, NULL, NULL, NULL, error);

  g_object_unref (client);

  return r;
}
