
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

#ifndef MPD_SHUTDOWN_NOTIFICATION_H
#define MPD_SHUTDOWN_NOTIFICATION_H

#include <glib-object.h>
#include <libnotify/notify.h>

G_BEGIN_DECLS

#define MPD_TYPE_SHUTDOWN_NOTIFICATION mpd_shutdown_notification_get_type()

#define MPD_SHUTDOWN_NOTIFICATION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_SHUTDOWN_NOTIFICATION, MpdShutdownNotification))

#define MPD_SHUTDOWN_NOTIFICATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_SHUTDOWN_NOTIFICATION, MpdShutdownNotificationClass))

#define MPD_IS_SHUTDOWN_NOTIFICATION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_SHUTDOWN_NOTIFICATION))

#define MPD_IS_SHUTDOWN_NOTIFICATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_SHUTDOWN_NOTIFICATION))

#define MPD_SHUTDOWN_NOTIFICATION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_SHUTDOWN_NOTIFICATION, MpdShutdownNotificationClass))

typedef struct
{
  NotifyNotification parent;
} MpdShutdownNotification;

typedef struct
{
  NotifyNotificationClass parent;
} MpdShutdownNotificationClass;

GType
mpd_shutdown_notification_get_type (void);

NotifyNotification *
mpd_shutdown_notification_new (char const *summary,
                               char const *body);

void
mpd_shutdown_notification_run (MpdShutdownNotification *self);

G_END_DECLS

#endif /* _MPD_SHUTDOWN_NOTIFICATION_H */

