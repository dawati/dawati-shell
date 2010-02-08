
/*
 * Copyright (c) 2010 Intel Corp.
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
 */

#include <gio/gio.h>
#include "mpd-folder-store.h"
#include "config.h"

#define MPD_FOLDER_STORE_ERROR mpd_folder_store_error_quark()

static GQuark
mpd_folder_store_error_quark (void)
{
  static GQuark _quark = 0;
  if (!_quark)
    _quark = g_quark_from_static_string ("mpd-folder-store-error");
  return _quark;
}

G_DEFINE_TYPE (MpdFolderStore, mpd_folder_store, CLUTTER_TYPE_LIST_MODEL)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_FOLDER_STORE, MpdFolderStorePrivate))

typedef struct
{
  int dummy;
} MpdFolderStorePrivate;

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_folder_store_parent_class)->dispose (object);
}

static void
mpd_folder_store_class_init (MpdFolderStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdFolderStorePrivate));

  object_class->dispose = _dispose;
}

static void
mpd_folder_store_init (MpdFolderStore *self)
{
}

ClutterModel *
mpd_folder_store_new (void)
{
  ClutterModel *model;
  GType         types[] = { G_TYPE_STRING, G_TYPE_STRING };

  model = g_object_new (MPD_TYPE_FOLDER_STORE, NULL);

  /* Column 0 ... URI
   * Column 1 ... Label or NULL */
  clutter_model_set_types (model, 2, types);

  return model;
}

gboolean
mpd_folder_store_add_user_directory (MpdFolderStore *self,
                                     GUserDirectory  directory)
{
  gchar const *path;
  gchar       *label;
  gchar       *uri;

  path = g_get_user_special_dir (directory);
  g_return_val_if_fail (path, FALSE);

  label = g_path_get_basename (path);
  uri = g_strdup_printf ("file://%s", path);

  clutter_model_append (CLUTTER_MODEL (self),
                        0, uri,
                        1, label,
                        -1);

  g_free (label);
  g_free (uri);

  return TRUE;
}

gboolean
mpd_folder_store_load_bookmarks_file (MpdFolderStore   *self,
                                      gchar const      *filename,
                                      GError          **error)
{
  gchar   *contents = NULL;
  gsize    length = 0;

  /* Load file. */

  if (!g_file_get_contents (filename, &contents, &length, error))
  {
    return FALSE;
  }

  if (length == 0)
  {
    if (error)
      *error = g_error_new (MPD_FOLDER_STORE_ERROR,
                            MPD_FOLDER_STORE_ERROR_BOOKMARKS_FILE_EMPTY,
                            "%s : Bookmarks file '%s' is empty",
                            G_STRLOC,
                            filename);
    return FALSE;
  }

  /* Parse content. */

  if (contents)
  {
    gchar **lines = g_strsplit (contents, "\n", -1);
    gchar **iter = lines;

    while (*iter)
    {
      gchar **line = g_strsplit (*iter, " ", 2);
      if (line && line[0])
      {
        const gchar *uri = line[0];
        gchar       *label = NULL;

        if (line[1])
          label = g_strdup (line[1]);
        else
          label = g_path_get_basename (uri);

        /* Insert into model. */
        clutter_model_append (CLUTTER_MODEL (self),
                              0, uri,
                              1, label,
                              -1);
        g_free (label);
      }
      g_strfreev (line);
      iter++;
    }
    g_strfreev (lines);
    g_free (contents);
  }

  return TRUE;
}

void
mpd_folder_store_clear (MpdFolderStore *self)
{
  while (clutter_model_get_n_rows (CLUTTER_MODEL (self)) > 0)
  {
    clutter_model_remove (CLUTTER_MODEL (self), 0);
  }
}

