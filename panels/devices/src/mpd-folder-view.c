
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

#include "mpd-folder-button.h"
#include "mpd-folder-view.h"
#include "config.h"

static void
_item_factory_iface_init (MxItemFactoryIface *iface);

G_DEFINE_TYPE_WITH_CODE (MpdFolderView, mpd_folder_view, MX_TYPE_ITEM_VIEW,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_ITEM_FACTORY, _item_factory_iface_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_FOLDER_VIEW, MpdFolderViewPrivate))

enum
{
  REQUEST_HIDE,

  LAST_SIGNAL
};

typedef struct
{
    int dummy;
} MpdFolderViewPrivate;

static guint _signals[LAST_SIGNAL] = { 0, };

/*
 * MpdFolderView.
 */

static void
_folder_button_clicked_cb (MpdFolderButton  *button,
                           MpdFolderView    *self)
{
  char const  *uri;
  char        *command_line;
  GError      *error = NULL;

  uri = mpd_folder_button_get_uri (button);
  command_line = g_strdup_printf ("nautilus %s", uri);
  g_spawn_command_line_async (command_line, &error);
  g_free (command_line);

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
  G_OBJECT_CLASS (mpd_folder_view_parent_class)->dispose (object);
}

static void
mpd_folder_view_class_init (MpdFolderViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdFolderViewPrivate));

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
mpd_folder_view_init (MpdFolderView *self)
{
  mx_item_view_set_factory (MX_ITEM_VIEW (self), MX_ITEM_FACTORY (self));
  mx_item_view_add_attribute (MX_ITEM_VIEW (self), "uri", 0);
  mx_item_view_add_attribute (MX_ITEM_VIEW (self), "label", 1);
  mx_item_view_add_attribute (MX_ITEM_VIEW (self), "icon-path", 2);
}

ClutterActor *
mpd_folder_view_new (void)
{
  return g_object_new (MPD_TYPE_FOLDER_VIEW, NULL);
}

/*
 * MxItemFactory interface.
 */

static ClutterActor *
_item_factory_create (MxItemFactory *factory)
{
  ClutterActor *folder_button;

  folder_button = mpd_folder_button_new ();
  g_signal_connect (folder_button, "clicked",
                    G_CALLBACK (_folder_button_clicked_cb), factory);

  return folder_button;
}

static void
_item_factory_iface_init (MxItemFactoryIface *iface)
{
  static gboolean _is_initialized = FALSE;

  if (!_is_initialized)
  {
    iface->create = _item_factory_create;
    _is_initialized = TRUE;
  }
}

