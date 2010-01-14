
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

G_DEFINE_TYPE (MpdFolderView, mpd_folder_view, MX_TYPE_ITEM_VIEW)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_FOLDER_VIEW, MpdFolderViewPrivate))

typedef struct
{
    int dummy;
} MpdFolderViewPrivate;

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
}

static void
mpd_folder_view_init (MpdFolderView *self)
{
  mx_item_view_set_item_type (MX_ITEM_VIEW (self), MPD_TYPE_FOLDER_BUTTON);
  mx_item_view_add_attribute (MX_ITEM_VIEW (self), "uri", 0);
  mx_item_view_add_attribute (MX_ITEM_VIEW (self), "label", 1);
}

ClutterActor *
mpd_folder_view_new (void)
{
  return g_object_new (MPD_TYPE_FOLDER_VIEW, NULL);
}


