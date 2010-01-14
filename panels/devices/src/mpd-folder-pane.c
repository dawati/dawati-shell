
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

G_DEFINE_TYPE (MpdFolderPane, mpd_folder_pane, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_FOLDER_PANE, MpdFolderPanePrivate))

typedef struct
{
  int dummy;
} MpdFolderPanePrivate;

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
}

static void
mpd_folder_pane_init (MpdFolderPane *self)
{
  ClutterModel  *store;
  ClutterActor  *label;
  ClutterActor  *view;
  gchar         *filename;
  GError        *error = NULL;

  mx_box_layout_set_vertical (MX_BOX_LAYOUT (self), TRUE);

  store = mpd_folder_store_new ();
  filename = g_build_filename (g_get_home_dir (), ".gtk-bookmarks", NULL);
  mpd_folder_store_load_file (MPD_FOLDER_STORE (store), filename, &error);
  g_free (filename);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  label = mx_label_new (_("File Browser"));
  clutter_container_add_actor (CLUTTER_CONTAINER (self), label);

  view = g_object_new (MPD_TYPE_FOLDER_VIEW, NULL);
  mx_item_view_set_model (MX_ITEM_VIEW (view), store);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), view);

  g_object_unref (store);
}

ClutterActor *
mpd_folder_pane_new (void)
{
  return g_object_new (MPD_TYPE_FOLDER_PANE, NULL);
}

