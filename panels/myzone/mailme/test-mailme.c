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
#include <glib-object.h>

#include "mailme-telepathy.h"
#include "mailme-telepathy-account.h"

static void
print_status (MailmeTelepathyAccount *account)
{
  gchar *display_name = NULL;
  guint unread_count = 0;
  
  g_object_get (account, "display-name", &display_name, NULL);
  g_object_get (account, "unread-count", &unread_count, NULL);

  g_debug ("%s: %i unread message(s)", display_name, unread_count);
  g_free (display_name);
}

static void
on_tp_provider_prepared (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  GMainLoop *loop = user_data;
  GError *error = NULL;
  if (!mailme_telepathy_prepare_finish (MAILME_TELEPATHY (source), result,
        &error))
  {
    g_debug ("Failed to prepare Telepathy provider: %s", error->message);
    g_main_loop_quit (loop);
    return;
  }
}

static void
on_account_changed (GObject *object,
    GParamSpec *pspec,
    gpointer user_data)
{
   print_status (MAILME_TELEPATHY_ACCOUNT (object));
}

static void
on_account_added (MailmeTelepathy *tp_provider,
    MailmeTelepathyAccount *account,
    gpointer user_data)
{
  gchar *display_name = NULL;
  g_object_get (account, "display-name", &display_name, NULL);
  g_debug ("Account %s added", display_name);
  g_free(display_name);
  print_status (account);

  g_signal_connect (G_OBJECT (account), "notify::unread-count",
      G_CALLBACK (on_account_changed), NULL);

  g_signal_connect (G_OBJECT (account), "notify::display-name",
      G_CALLBACK (on_account_changed), NULL);
}

static void
on_account_removed (MailmeTelepathy *tp_provider,
    MailmeTelepathyAccount *account,
    gpointer user_data)
{
  gchar *display_name = NULL;
  
  g_object_get (account, "display-name", &display_name, NULL);
  g_debug ("Account %s removed", display_name);
  g_free(display_name);

  g_signal_handlers_disconnect_by_func (account, on_account_changed, NULL);
}

gint main (gint argc, gchar **argv)
{
  GMainLoop *loop;
  GObject *tp_provider;

  g_type_init ();
  g_debug ("GLib initialized");

  loop = g_main_loop_new (NULL, FALSE);

  tp_provider = g_object_new (MAILME_TYPE_TELEPATHY, NULL);
  mailme_telepathy_prepare_async (MAILME_TELEPATHY (tp_provider),
      on_tp_provider_prepared, loop);

  g_signal_connect (G_OBJECT (tp_provider), "account-added",
      G_CALLBACK (on_account_added), NULL);

  g_signal_connect (G_OBJECT (tp_provider), "account-removed",
      G_CALLBACK (on_account_removed), NULL);

  g_main_loop_run (loop);

  g_object_unref (tp_provider);
  g_object_unref (loop);

	return 0;
}
