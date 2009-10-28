/*
 *
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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

#ifndef _PENGE_RECENT_FILES_MODEL
#define _PENGE_RECENT_FILES_MODEL

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define PENGE_TYPE_RECENT_FILE_MODEL penge_recent_files_model_get_type()

#define PENGE_RECENT_FILE_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_RECENT_FILE_MODEL, PengeRecentFilesModel))

#define PENGE_RECENT_FILE_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_RECENT_FILE_MODEL, PengeRecentFilesModelClass))

#define PENGE_IS_RECENT_FILE_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_RECENT_FILE_MODEL))

#define PENGE_IS_RECENT_FILE_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_RECENT_FILE_MODEL))

#define PENGE_RECENT_FILE_MODEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_RECENT_FILE_MODEL, PengeRecentFilesModelClass))

typedef struct {
  ClutterListModel parent;
} PengeRecentFilesModel;

typedef struct {
  ClutterListModelClass parent_class;
} PengeRecentFilesModelClass;

GType penge_recent_files_model_get_type (void);

ClutterModel *penge_recent_files_model_new (void);
void penge_recent_files_model_remove_item (PengeRecentFilesModel *model,
                                           GtkRecentInfo         *info);

G_END_DECLS

#endif /* _PENGE_RECENT_FILES_MODEL */

