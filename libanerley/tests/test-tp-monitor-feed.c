/*
 * Anerley - people feeds and widgets
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

#include <telepathy-glib/account-manager.h>
#include <telepathy-glib/contact.h>

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <anerley/anerley-aggregate-tp-feed.h>
#include <anerley/anerley-tp-monitor-feed.h>
#include <anerley/anerley-item.c>

static void
_unread_messages_changed (AnerleyItem *item,
                          guint        count)
{
  g_print ("Unread messages: %u\n", count);
}

static void
_items_added (AnerleyFeed *monitor,
              GList       *items)
{
  GList *ptr;

  for (ptr = items; ptr != NULL; ptr = ptr->next)
    {
      AnerleyItem *item = ANERLEY_ITEM (ptr->data);

      g_signal_connect (item, "unread-messages-changed",
          G_CALLBACK (_unread_messages_changed), NULL);
    }
}

static void
_items_removed (AnerleyFeed *monitor,
                GList       *items)
{
  GList *ptr;

  for (ptr = items; ptr != NULL; ptr = ptr->next)
    {
      AnerleyItem *item = ANERLEY_ITEM (ptr->data);

      g_signal_handlers_disconnect_by_func (item,
          G_CALLBACK (_unread_messages_changed), NULL);
    }
}

static void
_account_manager_ready_cb (GObject      *source_object,
                           GAsyncResult *result,
                           gpointer      userdata)

{
  AnerleyFeed *aggregate;
  AnerleyFeed *monitor;

  aggregate = anerley_aggregate_tp_feed_new ();
  monitor = anerley_tp_monitor_feed_new (ANERLEY_AGGREGATE_TP_FEED (aggregate),
                                         "AnerleyTest");
  g_signal_connect (monitor, "items-added",
      G_CALLBACK (_items_added), NULL);
  g_signal_connect (monitor, "items-removed",
      G_CALLBACK (_items_removed), NULL);
}

int
main (int    argc,
      char **argv)
{
  GMainLoop *main_loop;
  TpAccountManager *account_manager;

  g_type_init ();

  main_loop = g_main_loop_new (NULL, FALSE);

  account_manager = tp_account_manager_dup ();

  tp_proxy_prepare_async (TP_PROXY (account_manager),
                          NULL,
                          _account_manager_ready_cb,
                          NULL);
  g_main_loop_run (main_loop);

  g_object_unref (account_manager);

  return 0;
}
