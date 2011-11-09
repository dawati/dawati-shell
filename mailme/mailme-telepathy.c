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

#include <glib.h>
#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/dbus.h>

#include "mailme-telepathy.h"
#include "mailme-telepathy-account.h"

G_DEFINE_TYPE (MailmeTelepathy, mailme_telepathy, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
		(G_TYPE_INSTANCE_GET_PRIVATE ((o),\
				MAILME_TYPE_TELEPATHY, MailmeTelepathyPrivate))

typedef struct _MailmeTelepathyPrivate MailmeTelepathyPrivate;

struct _MailmeTelepathyPrivate {
	TpDBusDaemon *bus;
	TpAccountManager *manager;
  GHashTable *pending_accounts;
  GHashTable *accounts;
};

enum
{
  ACCOUNT_ADDED_SIGNAL,
  ACCOUNT_REMOVED_SIGNAL,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
on_account_status_changed (GObject    *object,
                           GParamSpec *pspec,
                           gpointer    user_data)
{
  MailmeTelepathyPrivate *priv = GET_PRIVATE (user_data);
  MailmeTelepathyAccount *mm_account = MAILME_TELEPATHY_ACCOUNT (object);
  TpAccount *tp_account =
    TP_ACCOUNT (mailme_telepathy_account_get_tp_account (mm_account));
  MailmeAccountStatus status;

  g_object_get (object, "status", &status, NULL);

  if (status == MAILME_ACCOUNT_DISCONNECTED
      && g_hash_table_lookup (priv->accounts, tp_account))
  {
    g_hash_table_remove (priv->accounts, tp_account);
    g_hash_table_insert (priv->pending_accounts, tp_account, mm_account);
    g_signal_emit (user_data, signals[ACCOUNT_REMOVED_SIGNAL], 0, mm_account);
  } else if (status == MAILME_ACCOUNT_SUPPORTED
      && g_hash_table_lookup (priv->pending_accounts, tp_account))
  {
    g_hash_table_remove (priv->pending_accounts, tp_account);
    g_hash_table_insert (priv->accounts, tp_account, mm_account);
    g_signal_emit (user_data, signals[ACCOUNT_ADDED_SIGNAL], 0, mm_account);
  }
}

static void
on_account_prepared (GObject      *source,
                     GAsyncResult *result,
                     gpointer      user_data)
{
  GError *error = NULL;
  MailmeAccountStatus status;
  MailmeTelepathy *self = MAILME_TELEPATHY (user_data);
  MailmeTelepathyPrivate *priv = GET_PRIVATE (self);
  MailmeTelepathyAccount *account = MAILME_TELEPATHY_ACCOUNT (source);
  TpAccount *tp_account =
    TP_ACCOUNT (mailme_telepathy_account_get_tp_account (account));

  status = mailme_telepathy_account_prepare_finish (account, result, &error);
  if (error != NULL)
  {
    g_warning ("An account failed to get prepared: %s", error->message);
		g_hash_table_remove (priv->pending_accounts, tp_account);
		g_object_unref (G_OBJECT (account));
    return;
  }

  g_signal_connect (account, "notify::status",
                    G_CALLBACK (on_account_status_changed),
                    self);

  switch (status)
  {
    case MAILME_ACCOUNT_UNSUPPORTED:
    case MAILME_ACCOUNT_DISCONNECTED:
      /* Keep in pending until something happens */
      break;
    case MAILME_ACCOUNT_SUPPORTED:
      /* Let's move this one to the list of supported */
      g_hash_table_remove (priv->pending_accounts, tp_account);
      g_hash_table_insert (priv->accounts, tp_account, account);
      g_signal_emit_by_name (user_data, "account-added", account);
      break;
    default:
      g_assert (!"Invalid status value");
      break;
  }
}

static void
foreach_account (TpAccount *tp_account,
                 gpointer   user_data)
{
  MailmeTelepathyPrivate *priv = GET_PRIVATE (user_data);
	MailmeTelepathyAccount *mm_account =
		g_object_new (MAILME_TYPE_TELEPATHY_ACCOUNT, NULL);

	mailme_telepathy_account_prepare_async (mm_account,
                                          G_OBJECT (tp_account),
                                          on_account_prepared,
                                          user_data);

  g_hash_table_insert (priv->pending_accounts, tp_account, mm_account);
}

static void
on_account_manager_prepared (GObject      *source,
                             GAsyncResult *result,
                             gpointer     user_data)
{
  GList *accounts;
  GError *error = NULL;
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
  MailmeTelepathy *self = MAILME_TELEPATHY (
		  g_async_result_get_source_object (G_ASYNC_RESULT (simple)));
  TpAccountManager *manager = TP_ACCOUNT_MANAGER (source);

  if (!tp_account_manager_prepare_finish (manager, result, &error))
  {
    g_simple_async_result_set_from_error (simple, error);
    g_simple_async_result_complete (simple);
    g_object_unref (self);
    return;
  }

  g_simple_async_result_complete (simple);

  accounts = tp_account_manager_get_valid_accounts (manager);
  if (accounts)
  {
    g_list_foreach (accounts, (GFunc) foreach_account, self);
  }

  g_object_unref (self);
}

static void
on_account_validity_changed (TpAccountManager *manager,
                             TpAccount        *tp_account,
                             gboolean          valid,
                             gpointer          user_data)
{
  MailmeTelepathyPrivate *priv = GET_PRIVATE (user_data);
  MailmeTelepathyAccount *mm_account;

  if (!valid)
  {
    GHashTable *source = priv->pending_accounts;

    mm_account = g_hash_table_lookup (source, tp_account);
    if (mm_account == NULL)
    {
      source = priv->accounts;
      mm_account = g_hash_table_lookup (source, tp_account);
      g_signal_emit_by_name (user_data, "account-removed", mm_account);
    }

    if (mm_account)
    {
      gchar *display_name;

      g_object_get (mm_account, "display-name", &display_name, NULL);
      g_free (display_name);

      g_hash_table_remove (source, tp_account);
      g_object_unref (mm_account);
    }
  }
  else
  {
    mm_account = g_hash_table_lookup (priv->pending_accounts, tp_account);
    if (mm_account == NULL)
      mm_account = g_hash_table_lookup (priv->accounts, tp_account);

    if (mm_account == NULL)
      foreach_account (tp_account, user_data);
  }
}
static void
on_account_removed (TpAccountManager *manager,
                    TpAccount        *tp_account,
                    gpointer          user_data)
{
  on_account_validity_changed (manager, tp_account, FALSE, user_data);
}


static void
foreach_account_unref (TpAccount *tp_account,
    MailmeTelepathyAccount *mm_account,
    gpointer user_data)
{
  g_object_unref (mm_account);
}

static void
mailme_telepathy_dispose (GObject *object)
{
	MailmeTelepathyPrivate *priv = GET_PRIVATE (object);

  if (priv->accounts)
  {
    g_hash_table_foreach (priv->accounts, (GHFunc)foreach_account_unref, NULL);
    g_hash_table_unref (priv->accounts);
    priv->accounts = NULL;
  }

  if (priv->pending_accounts)
  {
    g_hash_table_foreach (priv->pending_accounts,
        (GHFunc)foreach_account_unref, NULL);
    g_hash_table_unref (priv->pending_accounts);
    priv->pending_accounts = NULL;
  }

  if (priv->manager)
	{
		g_object_unref (priv->manager);
		priv->manager = NULL;
	}
	
  if (priv->bus)
	{
		g_object_unref (priv->bus);
		priv->bus = NULL;
	}
}

static void
mailme_telepathy_finalize (GObject *object)
{
	/* nothing to do */
}


static void
mailme_telepathy_class_init (MailmeTelepathyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MailmeTelepathyPrivate));

  object_class->dispose = mailme_telepathy_dispose;
  object_class->finalize = mailme_telepathy_finalize;

  signals[ACCOUNT_ADDED_SIGNAL] =
    g_signal_new ("account-added",
                  MAILME_TYPE_TELEPATHY,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1, MAILME_TYPE_TELEPATHY_ACCOUNT);

  signals[ACCOUNT_REMOVED_SIGNAL] =
    g_signal_new ("account-removed",
                  MAILME_TYPE_TELEPATHY,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1, MAILME_TYPE_TELEPATHY_ACCOUNT);
}

static void
mailme_telepathy_init (MailmeTelepathy *self)
{
	MailmeTelepathyPrivate *priv = GET_PRIVATE (self);
	priv->bus = NULL;
	priv->manager = NULL;
	priv->pending_accounts = g_hash_table_new (NULL, NULL);
	priv->accounts = g_hash_table_new (NULL, NULL);
}


void
mailme_telepathy_prepare_async (MailmeTelepathy     *self,
	                             	GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
	GSimpleAsyncResult *result;
	GError *error;
	MailmeTelepathyPrivate *priv = GET_PRIVATE (self);

	result = g_simple_async_result_new (G_OBJECT (self),
			callback, user_data, mailme_telepathy_prepare_finish);

	error = NULL;
	priv->bus = tp_dbus_daemon_dup (&error);
	if (error)
	{
		g_simple_async_result_set_from_error (result, error);
		g_simple_async_result_complete_in_idle (result);
		g_object_unref (result);
		return;
	}
	
	priv->manager = tp_account_manager_new (priv->bus);
	tp_account_manager_prepare_async (priv->manager,
                                    NULL,
                                    on_account_manager_prepared,
                                    result);

  g_signal_connect (priv->manager,
                    "account-validity-changed",
                    G_CALLBACK (on_account_validity_changed),
                    self);

  g_signal_connect (priv->manager,
                    "account-removed",
                    G_CALLBACK (on_account_removed),
                    self);
}

gboolean
mailme_telepathy_prepare_finish (MailmeTelepathy  *self,
                                 GAsyncResult     *result,
                                 GError          **error)
{
  GSimpleAsyncResult *simple;

  if (!g_simple_async_result_is_valid (result,
                                       G_OBJECT (self),
                                       mailme_telepathy_prepare_finish))
    return FALSE;

  simple = G_SIMPLE_ASYNC_RESULT (result);
  if (g_simple_async_result_propagate_error (simple, error))
    return FALSE;

  return TRUE;
}


