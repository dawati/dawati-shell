
#include <stdio.h>
#include <stdlib.h>

#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include "mnb-launcher-searchbar.h"

int
main (int argc, char *argv[])
{
  NbtkWidget *launcher;
  ClutterActor *stage;

  clutter_init (&argc, &argv);

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             "../data/theme/mutter-moblin.css", NULL);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 400, 200);

  launcher = mnb_launcher_searchbar_new ();
  clutter_actor_set_position (CLUTTER_ACTOR (launcher), 50, 50);
  clutter_actor_set_width (CLUTTER_ACTOR (launcher), 200);
  clutter_container_add (CLUTTER_CONTAINER (stage), CLUTTER_ACTOR (launcher), NULL);

  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
