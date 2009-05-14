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


#include <penge/penge-events-pane.h>

int
main (int    argc,
      char **argv)
{
  ClutterActor *stage;
  ClutterActor *pane;

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();
  pane = g_object_new (PENGE_TYPE_EVENTS_PANE, NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), pane);
  clutter_actor_show_all (stage);

  clutter_main ();
  return 0;
}
