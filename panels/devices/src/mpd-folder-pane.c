
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

#include <glib/gi18n.h>
#include "mpd-folder-pane.h"
#include "mpd-folder-store.h"
#include "mpd-folder-view.h"
#include "config.h"

G_DEFINE_TYPE (MpdFolderPane, mpd_folder_pane, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_FOLDER_PANE, MpdFolderPanePrivate))

enum
{
  REQUEST_HIDE,

  LAST_SIGNAL
};

typedef struct
{
  int dummy;
} MpdFolderPanePrivate;

static guint _signals[LAST_SIGNAL] = { 0, };

gchar *
uri_from_special_dir (GUserDirectory directory)
{
  gchar const *path;

  path = g_get_user_special_dir (directory);
  g_return_val_if_fail (path, NULL);

  return g_strdup_printf ("file://%s", path);
}

gchar *
icon_path_from_special_dir (GUserDirectory directory)
{
  gchar *icon_path;
  guint  i;

  static const struct {
    GUserDirectory   directory;
    gchar const     *key;
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
                                   ICONDIR,
                                   _map[i].key);
      break;
    }
  }

  if (icon_path == NULL)
  {
    icon_path = g_strdup_printf ("%s/directory-generic.png", ICONDIR);
  }

  return icon_path;
}

static void
_view_request_hide_cb (MpdFolderView  *folder_view,
                       MpdFolderPane  *self)
{
  g_signal_emit_by_name (self, "request-hide");
}

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_folder_pane_parent_class)->dispose (object);
}

static void
mpd_folder_pane_class_init (MpdFolderPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdFolderPanePrivate));

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
mpd_folder_pane_init (MpdFolderPane *self)
{
  GUserDirectory directories[] = { G_USER_DIRECTORY_DOCUMENTS,
                                   G_USER_DIRECTORY_DOWNLOAD,
                                   G_USER_DIRECTORY_MUSIC,
                                   G_USER_DIRECTORY_PICTURES,
                                   G_USER_DIRECTORY_VIDEOS };
  ClutterModel  *store;
  ClutterActor  *label;
  ClutterActor  *view;
  guint          i;

  mx_box_layout_set_vertical (MX_BOX_LAYOUT (self), TRUE);
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), 12);

  store = mpd_folder_store_new ();

  for (i = 0; i < G_N_ELEMENTS (directories); i++)
  {
    gchar *uri = uri_from_special_dir (directories[i]);
    gchar *icon_path = icon_path_from_special_dir (directories[i]);
    mpt_folder_store_add_directory (MPD_FOLDER_STORE (store), uri, icon_path);
    g_free (uri);
    g_free (icon_path);
  }

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

  label = mx_label_new (_("Your computer"));
  mx_stylable_set_style_class (MX_STYLABLE (label), "panel-title");
  clutter_container_add_actor (CLUTTER_CONTAINER (self), label);

  view = mpd_folder_view_new ();
  mx_item_view_set_model (MX_ITEM_VIEW (view), store);
  mx_grid_set_homogenous_rows (MX_GRID (view), TRUE);
  mx_grid_set_homogenous_columns (MX_GRID (view), TRUE);
  g_signal_connect (view, "request-hide",
                    G_CALLBACK (_view_request_hide_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), view);

  g_object_unref (store);
}

ClutterActor *
mpd_folder_pane_new (void)
{
  return g_object_new (MPD_TYPE_FOLDER_PANE, NULL);
}

