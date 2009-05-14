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


#include <penge/penge-recent-files-pane.h>

typedef struct
{
  ClutterActor *stage;
  ClutterActor *pane;
} RecentFilesPaneTestApp;

int
main (int    argc,
      char **argv)
{
  RecentFilesPaneTestApp *app;

  clutter_init (&argc, &argv);

  app = g_new0 (RecentFilesPaneTestApp, 1);

  app->stage = clutter_stage_get_default ();
  clutter_actor_set_size (app->stage, 700, 700);

  app->pane = g_object_new (PENGE_TYPE_RECENT_FILES_PANE, NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (app->stage), app->pane);
  clutter_actor_show_all (app->stage);
  clutter_main ();

  g_free (app);
}
