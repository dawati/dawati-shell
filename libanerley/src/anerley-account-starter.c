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

static void
_account_manager_ready_cb (TpAccountManager *am,
                           GAsyncResult     *res,
                           GMainLoop        *loop)
{
  GError *error = NULL;

  if (!tp_account_manager_prepare_finish (am, res, &error))
  {
    g_warning (G_STRLOC ": Error preparing account manager: %s",
               error->message);
    g_error_free (error);
    g_main_loop_quit (loop);
    return;
  }

  tp_account_manager_set_all_requested_presences (am,
                                                  TP_CONNECTION_PRESENCE_TYPE_AVAILABLE,
                                                  NULL,
                                                  NULL);
   g_main_loop_quit (loop);
}

int
main (int    argc,
      char **argv)
{
  GMainLoop *loop;
  TpAccountManager *am;

  g_type_init ();
  loop = g_main_loop_new (NULL, FALSE);

  am = tp_account_manager_dup ();
  tp_account_manager_prepare_async (am,
                                    NULL,
                                    (GAsyncReadyCallback)_account_manager_ready_cb,
                                    loop);
  g_main_loop_run (loop);

  return 0;
}
