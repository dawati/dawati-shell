/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _PENGE_APP_BOOKMARK_MANAGER
#define _PENGE_APP_BOOKMARK_MANAGER

#include <glib-object.h>

G_BEGIN_DECLS

#define PENGE_TYPE_APP_BOOKMARK_MANAGER penge_app_bookmark_manager_get_type()

#define PENGE_APP_BOOKMARK_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_APP_BOOKMARK_MANAGER, PengeAppBookmarkManager))

#define PENGE_APP_BOOKMARK_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_APP_BOOKMARK_MANAGER, PengeAppBookmarkManagerClass))

#define PENGE_IS_APP_BOOKMARK_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_APP_BOOKMARK_MANAGER))

#define PENGE_IS_APP_BOOKMARK_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_APP_BOOKMARK_MANAGER))

#define PENGE_APP_BOOKMARK_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_APP_BOOKMARK_MANAGER, PengeAppBookmarkManagerClass))

#define PENGE_TYPE_APP_BOOKMARK (penge_app_bookmark_get_type())

typedef struct {
  GObject parent;
} PengeAppBookmarkManager;

typedef struct {
  GObjectClass parent_class;
  void (*bookmark_added)(PengeAppBookmarkManager *manager, const gchar *uri);
  void (*bookmark_removed) (PengeAppBookmarkManager *manager, const gchar *uri);
} PengeAppBookmarkManagerClass;

GType penge_app_bookmark_manager_get_type (void);

void penge_app_bookmark_manager_add_uri (PengeAppBookmarkManager *manager,
                                         const gchar             *uri);
void penge_app_bookmark_manager_remove_uri (PengeAppBookmarkManager *manager,
                                            const gchar             *uri);

GList *penge_app_bookmark_manager_get_bookmarks (PengeAppBookmarkManager *manager);

void penge_app_bookmark_manager_save (PengeAppBookmarkManager *manager);
PengeAppBookmarkManager *penge_app_bookmark_manager_get_default (void);

G_END_DECLS

#endif /* _PENGE_APP_BOOKMARK_MANAGER */

