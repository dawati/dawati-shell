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

#ifndef __MAILME_TELEPATHY_ACCOUNT
#define __MAILME_TELEPATHY_ACCOUNT


#include <glib-object.h>
#include <gio/gio.h>


G_BEGIN_DECLS

#define MAILME_TYPE_TELEPATHY_ACCOUNT mailme_telepathy_account_get_type()

#define MAILME_TELEPATHY_ACCOUNT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILME_TYPE_TELEPATHY_ACCOUNT, MailmeTelepathyAccount))

#define MAILME_TELEPATHY_ACCOUNT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MAILME_TYPE_TELEPATHY_ACCOUNT, MailmeTelepathyAccountClass))

#define MAILME_IS_TELEPATHY_ACCOUNT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILME_TYPE_TELEPATHY_ACCOUNT))

#define MAILME_IS_TELEPATHY_ACCOUNT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MAILME_TYPE_TELEPATHY_ACCOUNT))

#define MAILME_TELEPATHY_ACCOUNT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MAILME_TYPE_TELEPATHY_ACCOUNT, MailmeTelepathyAccountClass))

typedef enum
{
  MAILME_ACCOUNT_DISCONNECTED,
  MAILME_ACCOUNT_UNSUPPORTED,
  MAILME_ACCOUNT_SUPPORTED,
  MAILME_ACCOUNT_NUM_STATUS
} MailmeAccountStatus;

typedef enum
{
  MAILME_INBOX_URI,
  MAILME_INBOX_COMMAND_LINE,
} MailmeInboxOpenFormat;

typedef struct {
  GObject parent;
} MailmeTelepathyAccount;

typedef struct {
  GObjectClass parent_class;
} MailmeTelepathyAccountClass;

GType mailme_telepathy_account_get_type (void);

void mailme_telepathy_account_prepare_async (MailmeTelepathyAccount *self,
		                                         GObject                *account,
                                             GAsyncReadyCallback     callback,
                                             gpointer                user_data);

MailmeAccountStatus mailme_telepathy_account_prepare_finish (
        MailmeTelepathyAccount *self,
        GAsyncResult           *result,
        GError                **error);

GObject *mailme_telepathy_account_get_tp_account (
        MailmeTelepathyAccount *self);

void mailme_telepathy_account_get_inbox_async (
                                             MailmeTelepathyAccount *self,
                                             GAsyncReadyCallback     callback,
                                             gpointer                user_data);

gchar *mailme_telepathy_account_get_inbox_finish (
                                             MailmeTelepathyAccount *self,
                                             GAsyncResult           *result,
                                             MailmeInboxOpenFormat  *format,
                                             GError                **error);
G_END_DECLS


#endif /* ifndef __MAILME_TELEPATHY_ACCOUNT */
