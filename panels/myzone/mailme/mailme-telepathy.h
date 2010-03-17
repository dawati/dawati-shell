/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
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

#ifndef __MAILME_TELEPATHY
#define __MAILME_TELEPATHY


#include <glib-object.h>
#include <gio/gio.h>


G_BEGIN_DECLS

#define MAILME_TYPE_TELEPATHY mailme_telepathy_get_type()

#define MAILME_TELEPATHY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILME_TYPE_TELEPATHY, MailmeTelepathy))

#define MAILME_TELEPATHY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MAILME_TYPE_TELEPATHY, MailmeTelepathyClass))

#define MAILME_IS_TELEPATHY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILME_TYPE_TELEPATHY))

#define MAILME_IS_TELEPATHY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MAILME_TYPE_TELEPATHY))

#define MAILME_TELEPATHY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MAILME_TYPE_TELEPATHY, MailmeTelepathyClass))

typedef struct {
  GObject parent;
} MailmeTelepathy;

typedef struct {
  GObjectClass parent_class;
} MailmeTelepathyClass;

GType mailme_telepathy_get_type (void);

void mailme_telepathy_prepare_async (MailmeTelepathy     *self,
		                             GAsyncReadyCallback  callback,
                                     gpointer             user_data);

gboolean mailme_telepathy_prepare_finish (MailmeTelepathy *self,
                                  		  GAsyncResult    *result,
                                          GError         **error);

G_END_DECLS

#endif /* ifndef __MAILME_TELEPATHY */
