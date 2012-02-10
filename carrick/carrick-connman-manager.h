/*
 * Carrick - a connection panel for the Dawati Netbook
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 * Author Jussi Kukkonen <jussi.kukkonen@intel.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef _CARRICK_CONNMAN_MANAGER_H
#define _CARRICK_CONNMAN_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define CARRICK_TYPE_CONNMAN_MANAGER carrick_connman_manager_get_type()

#define CARRICK_CONNMAN_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CARRICK_TYPE_CONNMAN_MANAGER, CarrickConnmanManager))

#define CARRICK_CONNMAN_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CARRICK_TYPE_CONNMAN_MANAGER, CarrickConnmanManagerClass))

#define CARRICK_IS_CONNMAN_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CARRICK_TYPE_CONNMAN_MANAGER))

#define CARRICK_IS_CONNMAN_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CARRICK_TYPE_CONNMAN_MANAGER))

#define CARRICK_CONNMAN_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CARRICK_TYPE_CONNMAN_MANAGER, CarrickConnmanManagerClass))

typedef struct _CarrickConnmanManager CarrickConnmanManager;
typedef struct _CarrickConnmanManagerClass CarrickConnmanManagerClass;
typedef struct _CarrickConnmanManagerPrivate CarrickConnmanManagerPrivate;

struct _CarrickConnmanManager
{
  GObject parent;

  CarrickConnmanManagerPrivate *priv;
};

struct _CarrickConnmanManagerClass
{
  GObjectClass parent_class;
};

GType carrick_connman_manager_get_type (void) G_GNUC_CONST;

void carrick_connman_manager_set_technology_state (CarrickConnmanManager *self, const char *tech, gboolean enabled);
CarrickConnmanManager *carrick_connman_manager_new (void);

G_END_DECLS

#endif /* _CARRICK_CONNMAN_MANAGER_H */
