/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Matthew Allum <mallum@linux.intel.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _MNB_NOTIFICATION
#define _MNB_NOTIFICATION

#include <glib-object.h>
#include <clutter/clutter.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MNB_TYPE_NOTIFICATION mnb_notification_get_type()

#define MNB_NOTIFICATION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_NOTIFICATION, MnbNotification))

#define MNB_NOTIFICATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_NOTIFICATION, MnbNotificationClass))

#define MNB_IS_NOTIFICATION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_NOTIFICATION))

#define MNB_IS_NOTIFICATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_NOTIFICATION))

#define MNB_NOTIFICATION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_NOTIFICATION, MnbNotificationClass))

typedef struct _MnbNotificationPrivate MnbNotificationPrivate;

typedef struct {
  NbtkTable parent;
  /*< private >*/
  MnbNotificationPrivate *priv;
} MnbNotification;

typedef struct {
  NbtkTableClass parent_class;

  void (*show_completed) (MnbNotification *notification);
} MnbNotificationClass;

GType mnb_notification_get_type (void);

NbtkWidget* mnb_notification_new (void);

G_END_DECLS

#endif /* _MNB_NOTIFICATION */


