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


/**
 * mpl_create_audio_store:
 *
 * Creates a new #GtkListStore to store audio items.
 *
 * Return value: (transfer full): a #GtkListStore
 */
GtkListStore *
mpl_create_audio_store (void)
{
  return gtk_list_store_new (5,
                             G_TYPE_STRING,  // Id
                             G_TYPE_STRING,  // Thumbnail
                             G_TYPE_STRING,  // Track name
                             G_TYPE_STRING,  // Artist name
                             G_TYPE_STRING); // Album name
}

/**
 * mpl_audio_store_set:
 * @store: a #GtkListStore
 * @iter: a #GtkTreeIter
 * @id: a unique id
 * @thumbnail: (allow-none): a URI to a thumbnail file
 * @track_name: (allow-none): track name
 * @artist_name: (allow-none): artist name
 * @album_name: (allow-none): album name
 *
 * Store a new line inside a GtkListStore.
 *
 */
void
mpl_audio_store_set (GtkListStore *store,
                     GtkTreeIter *iter,
                     const gchar *id,
                     const gchar *thumbnail,
                     const gchar *track_name,
                     const gchar *artist_name,
                     const gchar *album_name)
{
  gtk_list_store_set (store, iter,
                      0, id,
                      1, thumbnail,
                      2, track_name != NULL ? track_name : _("Unknown"),
                      3, artist_name != NULL ? artist_name : _("Unknown"),
                      4, album_name != NULL ? album_name : _("Unknown"),
                      -1);
}

gboolean
mpl_utils_panel_in_standalone_mode (void)
{
  static gboolean initialized = FALSE;
  static gboolean in_standalone;

  if (G_UNLIKELY (!initialized))
    {
      in_standalone = g_getenv ("DAWATI_PANEL_STANDALONE") != NULL;
    }

  return in_standalone;
}
