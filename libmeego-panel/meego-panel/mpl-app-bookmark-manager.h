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


#ifndef _MPL_APP_BOOKMARK_MANAGER
#define _MPL_APP_BOOKMARK_MANAGER

#include <glib-object.h>

G_BEGIN_DECLS

#define MPL_TYPE_APP_BOOKMARK_MANAGER mpl_app_bookmark_manager_get_type()

#define MPL_APP_BOOKMARK_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPL_TYPE_APP_BOOKMARK_MANAGER, MplAppBookmarkManager))

#define MPL_APP_BOOKMARK_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPL_TYPE_APP_BOOKMARK_MANAGER, MplAppBookmarkManagerClass))

#define MPL_IS_APP_BOOKMARK_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPL_TYPE_APP_BOOKMARK_MANAGER))

#define MPL_IS_APP_BOOKMARK_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPL_TYPE_APP_BOOKMARK_MANAGER))

#define MPL_APP_BOOKMARK_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPL_TYPE_APP_BOOKMARK_MANAGER, MplAppBookmarkManagerClass))

typedef struct _MplAppBookmarkManager      MplAppBookmarkManager;
typedef struct _MplAppBookmarkManagerClass MplAppBookmarkManagerClass;

/**
 * MplAppBookmarkManager:
 *
 * Manager for application bookmarks.
 */
struct _MplAppBookmarkManager
{
  /*<private>*/
  GObject parent;
};

/**
 * MplAppBookmarkManagerClass:
 * @bookmarks_changed: singnal closure for the
 * MplAppBookmarkManager::bookmarks-changed signal.
 *
 * Class struct for #MplAppBookmarkManager.
 */
struct _MplAppBookmarkManagerClass
{
  /*<private>*/
  GObjectClass parent_class;

  /*<public>*/
  void (*bookmarks_changed)(MplAppBookmarkManager *manager);
};

GType mpl_app_bookmark_manager_get_type (void);

void mpl_app_bookmark_manager_add_uri (MplAppBookmarkManager *manager,
                                       const gchar             *uri);
void mpl_app_bookmark_manager_remove_uri (MplAppBookmarkManager *manager,
                                          const gchar             *uri);

GList *mpl_app_bookmark_manager_get_bookmarks (MplAppBookmarkManager *manager);

void mpl_app_bookmark_manager_save (MplAppBookmarkManager *manager);
MplAppBookmarkManager *mpl_app_bookmark_manager_get_default (void);

G_END_DECLS

#endif /* _MPL_APP_BOOKMARK_MANAGER */

