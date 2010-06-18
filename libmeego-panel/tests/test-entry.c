
#include <stdio.h>
#include <stdlib.h>

#include <clutter/clutter.h>
#include <mx/mx.h>
#include <meego-panel/mpl-entry.h>

static void
button_clicked_cb (MplEntry *entry,
                   gpointer  user_data)
{
  printf ("%s() %s\n", __FUNCTION__, mpl_entry_get_text (entry));
}

int
main (int argc, char *argv[])
{
  MxWidget *entry;
  ClutterActor *stage;

  clutter_init (&argc, &argv);

  mx_style_load_from_file (mx_style_get_default (),
                             "../theme/theme.css", NULL);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 400, 200);

  entry = mpl_entry_new ("Foo");

  clutter_actor_set_width (CLUTTER_ACTOR (entry), 200);
  clutter_actor_set_position (CLUTTER_ACTOR (entry), 50, 50);
  clutter_container_add (CLUTTER_CONTAINER (stage), CLUTTER_ACTOR (entry), NULL);

  g_signal_connect (entry, "button-clicked", G_CALLBACK (button_clicked_cb), NULL);

  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
