/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Iain Holmes <iain@linux.intel.com>
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

#ifndef __PENGE_SOURCE_MANAGER_H__
#define __PENGE_SOURCE_MANAGER_H__

#include <glib-object.h>
#include <bickley/bkl-item.h>

G_BEGIN_DECLS

#define PENGE_TYPE_SOURCE_MANAGER                                      \
   (penge_source_manager_get_type())
#define PENGE_SOURCE_MANAGER(obj)                                      \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                PENGE_TYPE_SOURCE_MANAGER,             \
                                PengeSourceManager))
#define PENGE_SOURCE_MANAGER_CLASS(klass)                              \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             PENGE_TYPE_SOURCE_MANAGER,                \
                             PengeSourceManagerClass))
#define IS_PENGE_SOURCE_MANAGER(obj)                                   \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                PENGE_TYPE_SOURCE_MANAGER))
#define IS_PENGE_SOURCE_MANAGER_CLASS(klass)                           \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             PENGE_TYPE_SOURCE_MANAGER))
#define PENGE_SOURCE_MANAGER_GET_CLASS(obj)                            \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               PENGE_TYPE_SOURCE_MANAGER,              \
                               PengeSourceManagerClass))

typedef struct _PengeSourceManagerPrivate PengeSourceManagerPrivate;
typedef struct _PengeSourceManager      PengeSourceManager;
typedef struct _PengeSourceManagerClass PengeSourceManagerClass;

struct _PengeSourceManager
{
  GObject parent;

  PengeSourceManagerPrivate *priv;
};

struct _PengeSourceManagerClass
{
  GObjectClass parent_class;
};

GType penge_source_manager_get_type (void) G_GNUC_CONST;
BklItem *penge_source_manager_find_item (PengeSourceManager *self,
                                         const char          *uri);

G_END_DECLS

#endif /* __PENGE_SOURCE_MANAGER_H__ */
