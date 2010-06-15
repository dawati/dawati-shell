/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Authors: Tomas Frydrych <tf@linux.intel.com>
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

#ifndef _NTF_NOTIFICATION_H
#define _NTF_NOTIFICATION_H

#include <mx/mx.h>

#include "ntf-source.h"

G_BEGIN_DECLS

#define NTF_TYPE_NOTIFICATION                                           \
   (ntf_notification_get_type())
#define NTF_NOTIFICATION(obj)                                           \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                NTF_TYPE_NOTIFICATION,                  \
                                NtfNotification))
#define NTF_NOTIFICATION_CLASS(klass)                                   \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             NTF_TYPE_NOTIFICATION,                     \
                             NtfNotificationClass))
#define NTF_IS_NOTIFICATION(obj)                                        \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                NTF_TYPE_NOTIFICATION))
#define NTF_IS_NOTIFICATION_CLASS(klass)                                \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             NTF_TYPE_NOTIFICATION))
#define NTF_NOTIFICATION_GET_CLASS(obj)                                 \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               NTF_TYPE_NOTIFICATION,                   \
                               NtfNotificationClass))

typedef struct _NtfNotification        NtfNotification;
typedef struct _NtfNotificationClass   NtfNotificationClass;
typedef struct _NtfNotificationPrivate NtfNotificationPrivate;

struct _NtfNotificationClass
{
  MxTableClass parent_class;

  void (*closed) (NtfNotification *ntf);
};

struct _NtfNotification
{
  MxTable parent;

  /*<private>*/
  NtfNotificationPrivate *priv;
};

GType ntf_notification_get_type (void) G_GNUC_CONST;

NtfNotification *ntf_notification_new (NtfSource *src,
                                       gint       subsystem,
                                       gint       id,
                                       gboolean   no_dismiss_button);

NtfSource *ntf_notification_get_source         (NtfNotification *ntf);
void       ntf_notification_close              (NtfNotification *ntf);
void       ntf_notification_add_button         (NtfNotification *ntf,
                                                ClutterActor    *button,
                                                KeySym           keysym);
void       ntf_notification_remove_button      (NtfNotification *ntf,
                                                ClutterActor    *button);
void       ntf_notification_remove_all_buttons (NtfNotification *ntf);
void       ntf_notification_set_summary        (NtfNotification *ntf,
                                                const gchar     *text);
void       ntf_notification_set_body           (NtfNotification *ntf,
                                                const gchar     *text);
void       ntf_notification_set_icon           (NtfNotification *ntf,
                                                ClutterActor    *icon);
void       ntf_notification_set_timeout        (NtfNotification *ntf,
                                                guint            timeout);
gint       ntf_notification_get_id             (NtfNotification *ntf);
gint       ntf_notification_get_subsystem      (NtfNotification *ntf);
void       ntf_notification_set_urgent         (NtfNotification *ntf,
                                                gboolean is_urgent);

gint       ntf_notification_get_subsystem_id   (void);

gboolean   ntf_notification_handle_key_event   (NtfNotification *ntf,
                                                ClutterKeyEvent *event);
gboolean   ntf_notification_is_closed          (NtfNotification *ntf);

G_END_DECLS

#endif /* _NTF_NOTIFICATION_H */
