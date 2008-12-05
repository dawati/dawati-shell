
#include <clutter/clutter.h>
#include <libjana-ecal/jana-ecal.h>

#include <penge-day-tile.h>

int
main (int    argc,
      char **argv)
{
  ClutterActor *stage;
  ClutterActor *day_tile;
  JanaTime *now;

  clutter_init (&argc, &argv);

  now = jana_ecal_utils_time_now_local ();
  stage = clutter_stage_get_default ();
  day_tile = g_object_new (PENGE_TYPE_DAY_TILE,
                           "date",
                           now,
                           NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), day_tile);
  clutter_actor_set_size (day_tile, 100, 100);
  clutter_actor_show_all (stage);

  clutter_main ();

}
