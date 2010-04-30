
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "mpd-folder-button.h"
#include "mpd-folder-tile.h"
#include "config.h"

G_DEFINE_TYPE (MpdFolderTile, mpd_folder_tile, MX_TYPE_FRAME)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_FOLDER_TILE, MpdFolderTilePrivate))

enum
{
  REQUEST_HIDE,

  LAST_SIGNAL
};

typedef struct
{
  int dummy;
} MpdFolderTilePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static char *
uri_from_special_dir (GUserDirectory directory)
{
  char const *path;

  path = g_get_user_special_dir (directory);
  g_return_val_if_fail (path, NULL);

  return g_strdup_printf ("file://%s", path);
}

static char *
icon_path_from_special_dir (GUserDirectory directory)
{
  char          *icon_path;
  unsigned int   i;

  static const struct {
    GUserDirectory   directory;
    char const      *key;
  } _map[] = {
	  { G_USER_DIRECTORY_DOCUMENTS, "documents" },
	  { G_USER_DIRECTORY_DOWNLOAD, "download" },
	  { G_USER_DIRECTORY_MUSIC, "music" },
	  { G_USER_DIRECTORY_PICTURES, "pictures" },
	  { G_USER_DIRECTORY_VIDEOS, "videos" }
  };

  icon_path = NULL;
  for (i = 0; i < G_N_ELEMENTS (_map); i++)
  {
    if (directory == _map[i].directory)
    {
      icon_path = g_strdup_printf ("%s/directory-%s.png",
                                   PKGICONDIR,
                                   _map[i].key);
      break;
    }
  }

  if (icon_path == NULL)
  {
    icon_path = g_strdup_printf ("%s/directory-generic.png", PKGICONDIR);
  }

  return icon_path;
}

static void
_button_clicked_cb (MpdFolderButton  *button,
                    MpdFolderTile    *self)
{
  char const  *uri;
  GError      *error = NULL;

  uri = mpd_folder_button_get_uri (button);
  gtk_show_uri (NULL, uri, clutter_get_current_event_time (), &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  } else {
    g_signal_emit_by_name (self, "request-hide");
  }
}

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_folder_tile_parent_class)->dispose (object);
}

static void
mpd_folder_tile_class_init (MpdFolderTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdFolderTilePrivate));

  object_class->dispose = _dispose;

  /* Signals */

  _signals[REQUEST_HIDE] = g_signal_new ("request-hide",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);
}

static void
mpd_folder_tile_init (MpdFolderTile *self)
{
  GUserDirectory directories[] = { G_USER_DIRECTORY_DOCUMENTS,
                                   G_USER_DIRECTORY_DOWNLOAD,
                                   G_USER_DIRECTORY_MUSIC,
                                   G_USER_DIRECTORY_PICTURES,
                                   G_USER_DIRECTORY_VIDEOS };
  ClutterActor  *box;
  ClutterActor  *button;
  unsigned int   i;

  box = clutter_box_new (clutter_flow_layout_new (CLUTTER_FLOW_HORIZONTAL));
  mx_bin_set_child (MX_BIN (self), box);
  mx_bin_set_alignment (MX_BIN (self), MX_ALIGN_MIDDLE, MX_ALIGN_START);

  for (i = 0; i < G_N_ELEMENTS (directories); i++)
  {
    char *uri = uri_from_special_dir (directories[i]);
    char *label = g_path_get_basename (uri);
    char *icon_path = icon_path_from_special_dir (directories[i]);

    button = g_object_new (MPD_TYPE_FOLDER_BUTTON,
                           "uri", uri,
                           "label", label,
                           "icon-path", icon_path,
                           NULL);
    g_signal_connect (button, "clicked",
                      G_CALLBACK (_button_clicked_cb), self);
    clutter_container_add_actor (CLUTTER_CONTAINER (box), button);

    g_free (icon_path);
    g_free (label);
    g_free (uri);
  }

  /* Add trash bin. */
  button = g_object_new (MPD_TYPE_FOLDER_BUTTON,
                         "uri", "trash:///",
                         "label", _("Trash"),
                         "icon-path", PKGICONDIR "/directory-trash.png",
                         NULL);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (_button_clicked_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (box), button);

#if 0 /* Not showing gtk-bookmarks for now. */
  filename = g_build_filename (g_get_home_dir (), ".gtk-bookmarks", NULL);
  mpd_folder_store_load_bookmarks_file (MPD_FOLDER_STORE (store),
                                        filename,
                                        &error);
  g_free (filename);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
#endif
}

ClutterActor *
mpd_folder_tile_new (void)
{
  return g_object_new (MPD_TYPE_FOLDER_TILE, NULL);
}

