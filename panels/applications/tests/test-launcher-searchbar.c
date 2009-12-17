
#include <stdio.h>
#include <stdlib.h>

#include <clutter/clutter.h>
#include <mx/mx.h>
#include "mnb-launcher-searchbar.h"

static void
search_cb (MnbLauncherSearchbar *bar)
{
  gchar *text;

  text = NULL;
  g_object_get (bar, "text", &text, NULL);

  printf ("%s\n", text);

  g_free (text);
}

int
main (int argc, char *argv[])
{
  MxWidget     *bar;
  ClutterActor *stage;

  clutter_init (&argc, &argv);

  mx_style_load_from_file (mx_style_get_default (),
                           "../theme/panel.css", NULL);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 400, 200);

  bar = mnb_launcher_searchbar_new ();
  clutter_actor_set_position (CLUTTER_ACTOR (bar), 50, 50);
  clutter_container_add (CLUTTER_CONTAINER (stage), CLUTTER_ACTOR (bar), NULL);
  g_signal_connect (bar, "activated", G_CALLBACK (search_cb), NULL);

  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
