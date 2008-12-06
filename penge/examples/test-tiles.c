
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
