
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

#ifndef MPD_FOLDER_BUTTON_H
#define MPD_FOLDER_BUTTON_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MPD_TYPE_FOLDER_BUTTON mpd_folder_button_get_type()

#define MPD_FOLDER_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_FOLDER_BUTTON, MpdFolderButton))

#define MPD_FOLDER_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_FOLDER_BUTTON, MpdFolderButtonClass))

#define MPD_IS_FOLDER_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_FOLDER_BUTTON))

#define MPD_IS_FOLDER_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_FOLDER_BUTTON))

#define MPD_FOLDER_BUTTON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_FOLDER_BUTTON, MpdFolderButtonClass))

typedef struct
{
  MxButton parent;
} MpdFolderButton;

typedef struct
{
  MxButtonClass parent;
} MpdFolderButtonClass;

GType
mpd_folder_button_get_type (void);

ClutterActor *
mpd_folder_button_new     (void);

gchar const *
mpd_folder_button_get_icon_path (MpdFolderButton *self);

void
mpd_folder_button_set_icon_path (MpdFolderButton *self,
                                 gchar const    *icon_path);

gchar const *
mpd_folder_button_get_uri (MpdFolderButton const  *self);

void
mpd_folder_button_set_uri (MpdFolderButton        *self,
                           gchar const            *uri);

G_END_DECLS

#endif /* MPD_FOLDER_BUTTON_H */

