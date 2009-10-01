/*
 * Dalston - power and volume applets for Moblin
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
 *
 */

#ifndef _DALSTON_IDLE_MANAGER
#define _DALSTON_IDLE_MANAGER

#include <glib-object.h>

G_BEGIN_DECLS

#define DALSTON_TYPE_IDLE_MANAGER dalston_idle_manager_get_type()

#define DALSTON_IDLE_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DALSTON_TYPE_IDLE_MANAGER, DalstonIdleManager))

#define DALSTON_IDLE_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DALSTON_TYPE_IDLE_MANAGER, DalstonIdleManagerClass))

#define DALSTON_IS_IDLE_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DALSTON_TYPE_IDLE_MANAGER))

#define DALSTON_IS_IDLE_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DALSTON_TYPE_IDLE_MANAGER))

#define DALSTON_IDLE_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DALSTON_TYPE_IDLE_MANAGER, DalstonIdleManagerClass))

typedef struct {
  GObject parent;
} DalstonIdleManager;

typedef struct {
  GObjectClass parent_class;
} DalstonIdleManagerClass;

GType dalston_idle_manager_get_type (void);

DalstonIdleManager *dalston_idle_manager_new (void);

G_END_DECLS

#endif /* _DALSTON_IDLE_MANAGER */

