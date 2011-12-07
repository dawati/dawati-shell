/*
 * Anerley - people feeds and widgets
 * Copyright (C) 2010, Intel Corporation.
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

#include <telepathy-glib/telepathy-glib.h>
#include "sw-online.h"

static void
_account_manager_ready_cb (TpAccountManager *am,
                           GAsyncResult     *res,
                           gpointer          userdata)
{
  TpConnectionPresenceType type = (TpConnectionPresenceType)GPOINTER_TO_INT (userdata);
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (TP_PROXY (am), res, &error))
  {
    g_warning (G_STRLOC ": Error preparing account manager: %s",
               error->message);
    g_error_free (error);
    return;
  }

  tp_account_manager_set_all_requested_presences (am,
                                                  type,
                                                  NULL,
                                                  NULL);
}

static void
changed_accounts (TpConnectionPresenceType type)
{
  TpAccountManager *am;

  am = tp_account_manager_dup ();
  tp_proxy_prepare_async (TP_PROXY (am),
                          NULL,
                          (GAsyncReadyCallback)_account_manager_ready_cb,
                          GINT_TO_POINTER (type));
  g_object_unref (am);
}

static void
_online_notify_cb (gboolean online,
                   gpointer userdata)
{
  g_debug (G_STRLOC ": Online = %s", online ? "yes" : "no");
  if (online)
    changed_accounts (TP_CONNECTION_PRESENCE_TYPE_AVAILABLE);
  else
    changed_accounts (TP_CONNECTION_PRESENCE_TYPE_OFFLINE);
}

int
main (int    argc,
      char **argv)
{
  GMainLoop *loop;
  gboolean online;

  g_type_init ();
  loop = g_main_loop_new (NULL, FALSE);

  sw_online_add_notify (_online_notify_cb, NULL);

  online = sw_is_online ();
  if (online)
  {
    g_debug (G_STRLOC ": Online = %s", online ? "yes" : "no");
    changed_accounts (TP_CONNECTION_PRESENCE_TYPE_AVAILABLE);
  }

  g_main_loop_run (loop);

  return 0;
}
