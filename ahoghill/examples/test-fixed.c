#include <nbtk/nbtk.h>
#include <ahoghill/nbtk-fixed.h>

int
main (int    argc,
      char **argv)
{
    ClutterActor *stage;
    ClutterActor *fixed;
    ClutterActor *rect;
    ClutterColor red = {0xff, 0x00, 0x00, 0xff};

    clutter_init (&argc, &argv);
    stage = clutter_stage_get_default ();
    clutter_actor_set_size (stage, 1024, 600);

    fixed = g_object_new (NBTK_TYPE_FIXED, NULL);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), fixed);
    clutter_actor_set_size (fixed, 824, 400);
    clutter_actor_set_position (fixed, 100, 100);

    rect = clutter_rectangle_new_with_color (&red);
    clutter_actor_set_size (rect, 200, 200);
    nbtk_fixed_add_actor (NBTK_FIXED (fixed), rect);
    clutter_actor_set_position (rect, -100, -100);
    clutter_actor_show (rect);

    clutter_actor_show_all (stage);

    clutter_main ();

    return 0;
}
