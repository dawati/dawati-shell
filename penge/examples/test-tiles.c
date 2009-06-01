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



#include <clutter/clutter.h>
#include <libjana-ecal/jana-ecal.h>

#include <penge-day-tile.h>
#include <penge-recent-file-tile.h>

int
main (int    argc,
      char **argv)
{
  ClutterActor *stage;
  ClutterActor *day_tile;
  JanaTime *now;
  ClutterActor *tile;
  ClutterActor *rect;
  ClutterColor reddish_color = { 0xdc, 0x00, 0x00, 0xdc };

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();

  now = jana_ecal_utils_time_now_local ();
  day_tile = g_object_new (PENGE_TYPE_DAY_TILE,
                           "date",
                           now,
                           NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), day_tile);
  clutter_actor_set_size (day_tile, 150, 150);
  clutter_actor_set_position (day_tile, 100, 100);

  rect = clutter_rectangle_new_with_color (&reddish_color);
  tile = g_object_new (PENGE_TYPE_TILE,
                       "child",
                       rect,
                       NULL);
  clutter_actor_set_size (tile, 100, 100);
  clutter_actor_set_size (rect, 50, 50);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), tile);
  clutter_actor_set_position (tile, 200, 200);

  rect = clutter_rectangle_new_with_color (&reddish_color);
  tile = g_object_new (PENGE_TYPE_TILE,
                       "child",
                       rect,
                       NULL);
  clutter_actor_set_size (rect, 100, 100);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), tile);
  clutter_actor_set_position (tile, 300, 300);

  tile = g_object_new (PENGE_TYPE_RECENT_FILE_TILE,
                       "uri",
                       "file:///home/rob/Desktop/drawing.svg",
                       NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), tile);
  clutter_actor_set_position (tile, 400, 100);

  clutter_actor_show_all (stage);
  clutter_main ();

}
