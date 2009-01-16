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
