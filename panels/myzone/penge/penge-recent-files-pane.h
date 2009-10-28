/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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


#ifndef _PENGE_RECENT_FILES_PANE
#define _PENGE_RECENT_FILES_PANE

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define PENGE_TYPE_RECENT_FILES_PANE penge_recent_files_pane_get_type()

#define PENGE_RECENT_FILES_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_RECENT_FILES_PANE, PengeRecentFilesPane))

#define PENGE_RECENT_FILES_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_RECENT_FILES_PANE, PengeRecentFilesPaneClass))

#define PENGE_IS_RECENT_FILES_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_RECENT_FILES_PANE))

#define PENGE_IS_RECENT_FILES_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_RECENT_FILES_PANE))

#define PENGE_RECENT_FILES_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_RECENT_FILES_PANE, PengeRecentFilesPaneClass))

typedef struct {
  NbtkTable parent;
} PengeRecentFilesPane;

typedef struct {
  NbtkTableClass parent_class;
} PengeRecentFilesPaneClass;

GType penge_recent_files_pane_get_type (void);

G_END_DECLS

#endif /* _PENGE_RECENT_FILES_PANE */

