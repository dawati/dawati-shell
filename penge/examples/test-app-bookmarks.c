/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <penge/penge-app-bookmark-manager.h>

#define DEVHELP_DESKTOP_URI "file:///usr/share/applications/devhelp.desktop"
#define EVOLUTION_DESKTOP_URI "file:///usr/share/applications/evolution.desktop"
#define EVINCE_DESKTOP_URI "file:///usr/share/applications/evince.desktop"

static void
_bookmark_added_cb (PengeAppBookmarkManager *manager,
                    const gchar             *uri)
{
  g_debug (G_STRLOC ": uri added: %s",
           uri);
}

static gboolean
_test_idle_cb (PengeAppBookmarkManager *manager)
{
  penge_app_bookmark_manager_add_uri (manager,
                                      EVINCE_DESKTOP_URI);

  return FALSE;
}

int
main (int     argc,
      char  **argv)
{
  PengeAppBookmarkManager *manager;
  GList *l;
  GMainLoop *loop;
  gchar *bookmark;

  g_type_init ();


  loop = g_main_loop_new (NULL, TRUE);
  manager = g_object_new (PENGE_TYPE_APP_BOOKMARK_MANAGER,
                          NULL);

  penge_app_bookmark_manager_add_uri (manager,
                                      DEVHELP_DESKTOP_URI);

  penge_app_bookmark_manager_add_uri (manager,
                                      EVOLUTION_DESKTOP_URI);


  for (l = penge_app_bookmark_manager_get_bookmarks (manager); l; l = l->next)
  {
    bookmark = (gchar *)l->data;
    g_debug ("%s", bookmark);
  }

  g_signal_connect (manager,
                    "bookmark-added",
                    _bookmark_added_cb,
                    NULL);

  g_idle_add (_test_idle_cb, manager);
  g_main_loop_run (loop);

  return 0;
}
