/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <glib/gi18n.h>

#include "mpl-utils.h"

/**
 * SECTION:mpl-utils
 * @short_description: Various utility functions and macros.
 * @Title: Utility Functions and Macros
 *
 * Miscellaneous utility functions and macros for Panels.
 */

/**
 * mpl_utils_get_thumbnail_path:
 * @uri: image uri
 *
 * Retrieves the path to thumbnail for an image identified by uri. The
 * thumbnails are searched for in ~/.bk-thumbnails, ~/thumbnails/large and
 * ~/thumbnails/normal, in that order.
 *
 * Return value: path to the thumbnail, or %NULL if thumbnail does not exist.
 * The retured string must be freed with g_free() when no longer needed.
 */
gchar *
mpl_utils_get_thumbnail_path (const gchar *uri)
{
  gchar *thumbnail_path;
  gchar *thumbnail_filename = NULL;
  gchar *csum;

  csum = g_compute_checksum_for_string (G_CHECKSUM_MD5, uri, -1);

  thumbnail_path = g_build_filename (g_get_home_dir (),
                                     ".bkl-thumbnails",
                                     csum,
                                     NULL);

  if (g_file_test (thumbnail_path, G_FILE_TEST_EXISTS))
  {
    g_free (csum);
    goto success;
  }

  g_free (thumbnail_path);

  thumbnail_filename = g_strconcat (csum, ".png", NULL);
  thumbnail_path = g_build_filename (g_get_home_dir (),
                                     ".thumbnails",
                                     "large",
                                     thumbnail_filename,
                                     NULL);

  g_free (csum);

  if (g_file_test (thumbnail_path, G_FILE_TEST_EXISTS))
  {
    goto success;
  } else {
    g_free (thumbnail_path);
    thumbnail_path = g_build_filename (g_get_home_dir (),
                                       ".thumbnails",
                                       "normal",
                                       thumbnail_filename,
                                       NULL);

    if (g_file_test (thumbnail_path, G_FILE_TEST_EXISTS))
    {
      goto success;
    }
  }

  g_free (thumbnail_filename);
  g_free (thumbnail_path);
  return NULL;

success:
  g_free (thumbnail_filename);
  return thumbnail_path;
}

