/*
 * Carrick - a connection panel for the MeeGo Netbook
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
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
 *
 * Written by - Joshua Lock <josh@linux.intel.com>
 *
 */

#ifndef _CARRICK_NOTIFICATION_MANAGER_H
#define _CARRICK_NOTIFICATION_MANAGER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CARRICK_TYPE_NOTIFICATION_MANAGER carrick_notification_manager_get_type ()

#define CARRICK_NOTIFICATION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                               CARRICK_TYPE_NOTIFICATION_MANAGER, CarrickNotificationManager))

#define CARRICK_NOTIFICATION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
                            CARRICK_TYPE_NOTIFICATION_MANAGER, CarrickNotificationManagerClass))

#define CARRICK_IS_NOTIFICATION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                               CARRICK_TYPE_NOTIFICATION_MANAGER))

#define CARRICK_IS_NOTIFICATION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                            CARRICK_TYPE_NOTIFICATION_MANAGER))

#define CARRICK_NOTIFICATION_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                              CARRICK_TYPE_NOTIFICATION_MANAGER, CarrickNotificationManagerClass))

typedef struct _CarrickNotificationManager CarrickNotificationManager;
typedef struct _CarrickNotificationManagerClass CarrickNotificationManagerClass;
typedef struct _CarrickNotificationManagerPrivate CarrickNotificationManagerPrivate;

struct _CarrickNotificationManager
{
  GObject parent;

  CarrickNotificationManagerPrivate *priv;
};

struct _CarrickNotificationManagerClass
{
  GObjectClass parent_class;
};

GType carrick_notification_manager_get_type (void);

CarrickNotificationManager *carrick_notification_manager_new ();

void carrick_notification_manager_queue_event (CarrickNotificationManager *self, const gchar *type, const gchar *state, const gchar *name);
void carrick_notification_manager_notify_event (CarrickNotificationManager *self, const gchar *type, const gchar *state, const gchar *name, guint str);

G_END_DECLS

#endif /* _CARRICK_NOTIFICATION_MANAGER_H */
