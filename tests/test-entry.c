
#include <stdio.h>
#include <stdlib.h>

#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include "mnb-entry.h"

static void
button_clicked_cb (MnbEntry *entry,
                   gpointer  user_data)
{
  printf ("%s() %s\n", __FUNCTION__, mnb_entry_get_text (entry));
}

int
main (int argc, char *argv[])
{
  NbtkWidget *entry;
  ClutterActor *stage;

  clutter_init (&argc, &argv);

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             "../data/theme/mutter-moblin.css", NULL);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 400, 200);

  entry = mnb_entry_new ("Foo");
/*
  entry = (NbtkWidget *) g_object_new (MNB_TYPE_ENTRY, 
                                       "text", "foo",
                                       "label", "bar",
                                       NULL);
*/
  clutter_actor_set_widthu (CLUTTER_ACTOR (entry), CLUTTER_UNITS_FROM_DEVICE (200));
  clutter_actor_set_position (CLUTTER_ACTOR (entry), 50, 50);
  clutter_container_add (CLUTTER_CONTAINER (stage), CLUTTER_ACTOR (entry), NULL);
  g_signal_connect (entry, "button-clicked", G_CALLBACK (button_clicked_cb), NULL);

  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
