
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
  ClutterModel  *store;
  ClutterActor  *label;
  ClutterActor  *view;
  gchar         *filename;
  GError        *error = NULL;

  mx_box_layout_set_vertical (MX_BOX_LAYOUT (self), TRUE);

  store = mpd_folder_store_new ();
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

  label = mx_label_new (_("File Browser"));
  clutter_container_add_actor (CLUTTER_CONTAINER (self), label);

  view = mpd_folder_view_new ();
  mx_item_view_set_model (MX_ITEM_VIEW (view), store);
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

