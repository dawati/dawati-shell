
#include <stdio.h>
#include <stdlib.h>

#include <clutter/clutter.h>
#include <mx/mx.h>
#include "mnb-filter.h"

int
main (int     argc,
      char  **argv)
{
  ClutterActor  *stage;
  ClutterActor  *filter;

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();

  filter = mnb_filter_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), filter);

  clutter_actor_show_all (stage);
  clutter_main ();

  return EXIT_SUCCESS;
}
