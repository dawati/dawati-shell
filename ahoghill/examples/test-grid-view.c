/* Cunningly borrowed from Penge */
#include <ahoghill/ahoghill-grid-view.h>

int
main (int    argc,
      char **argv)
{
    ClutterActor *stage;
    ClutterActor *grid;

    g_thread_init (NULL);
    bkl_init (&argc, &argv);
    clutter_init (&argc, &argv);

    srand (time (NULL));
    penge_utils_load_stylesheet ();

    stage = clutter_stage_get_default ();
    clutter_actor_set_size (stage, 1024, 600);

    grid = g_object_new (AHOGHILL_TYPE_GRID_VIEW, NULL);
    clutter_actor_set_size (grid, 1024, 500);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), grid);
    clutter_actor_set_position (grid, 0, 50);
    clutter_actor_show_all (stage);

    clutter_main ();

    return 0;
}
