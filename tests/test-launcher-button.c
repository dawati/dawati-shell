
#include <stdio.h>
#include <stdlib.h>

#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include "mnb-launcher-button.h"

static void
clicked_cb (NbtkWidget          *widget,
            ClutterButtonEvent  *event,
            gpointer             data)
{
  printf ("%s() %d\n", __FUNCTION__, event->time);
}

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

  launcher = mnb_launcher_button_new ("../data/theme/panel/internet-coloured.png",
                                      32, "Launcher Button Launcher Button ", 
                                      "Category", "Test", "Comment", "/bin/false");
  clutter_actor_set_position (CLUTTER_ACTOR (launcher), 50, 50);
  clutter_actor_set_width (CLUTTER_ACTOR (launcher), 200);
  clutter_container_add (CLUTTER_CONTAINER (stage), CLUTTER_ACTOR (launcher), NULL);
  g_signal_connect (launcher, "activated", G_CALLBACK (clicked_cb), NULL);

  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
