
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

#ifndef MPD_FOLDER_STORE_H
#define MPD_FOLDER_STORE_H

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MPD_TYPE_FOLDER_STORE mpd_folder_store_get_type()

#define MPD_FOLDER_STORE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_FOLDER_STORE, MpdFolderStore))

#define MPD_FOLDER_STORE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_FOLDER_STORE, MpdFolderStoreClass))

#define MPD_IS_FOLDER_STORE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_FOLDER_STORE))

#define MPD_IS_FOLDER_STORE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_FOLDER_STORE))

#define MPD_FOLDER_STORE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_FOLDER_STORE, MpdFolderStoreClass))

typedef struct
{
  ClutterListModel parent;
} MpdFolderStore;

typedef struct
{
  ClutterListModelClass parent_class;
} MpdFolderStoreClass;

enum
{
  MPD_FOLDER_STORE_ERROR_BOOKMARKS_FILE_EMPTY
};

GType
mpd_folder_store_get_type (void);

ClutterModel *
mpd_folder_store_new (void);

void
mpt_folder_store_add_directory (MpdFolderStore  *self,
                                gchar const     *uri,
                                gchar const     *icon_path);

gboolean
mpd_folder_store_load_bookmarks_file (MpdFolderStore   *self,
                                      gchar const      *filename,
                                      GError          **error);

void
mpd_folder_store_clear (MpdFolderStore *self);

G_END_DECLS

#endif /* _MPD_FOLDER_STORE_H */

