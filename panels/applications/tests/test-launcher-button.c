
#include <stdio.h>
#include <stdlib.h>

#include <clutter/clutter.h>
#include <mx/mx.h>
#include "mnb-launcher-button.h"

static void
clicked_cb (MxWidget            *widget,
            ClutterButtonEvent  *event,
            gpointer             data)
{
  printf ("%s() %d\n", __FUNCTION__, event->time);
}

static void
fav_toggled_cb (MnbLauncherButton *launcher,
                gpointer           data)
{
  printf ("%s() %d\n", __FUNCTION__, mnb_launcher_button_get_favorite (launcher));
}

int
main (int argc, char *argv[])
{
  MxWidget     *launcher;
  ClutterActor *stage;

  clutter_init (&argc, &argv);

  mx_style_load_from_file (mx_style_get_default (),
                           "../data/theme/panel.css", NULL);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 400, 200);

  launcher = mnb_launcher_button_new ("internet-coloured", "../data/theme/apps-coloured.png",
                                      32, "Launcher Button Launcher Button ", 
                                      "Category", "Comment", "/bin/false",
                                      "/usr/share/applications/eog.desktop");
  clutter_actor_set_position (CLUTTER_ACTOR (launcher), 50, 50);
  clutter_actor_set_width (CLUTTER_ACTOR (launcher), 200);
  clutter_container_add (CLUTTER_CONTAINER (stage), CLUTTER_ACTOR (launcher), NULL);
  g_signal_connect (launcher, "activated", G_CALLBACK (clicked_cb), NULL);
  g_signal_connect (launcher, "fav-toggled", G_CALLBACK (fav_toggled_cb), NULL);

  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
