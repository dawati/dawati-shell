
#include <stdio.h>
#include <stdlib.h>

#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include "mnb-status-entry.h"

static void
status_changed_cb (MnbStatusEntry *entry,
                   const gchar    *status,
                   gpointer        user_data)
{
  printf ("%s() %s\n", __FUNCTION__, status);
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

  entry = mnb_status_entry_new ("foo");
  mnb_status_entry_show_button (MNB_STATUS_ENTRY (entry), TRUE);
  clutter_actor_set_position (CLUTTER_ACTOR (entry), 50, 50);
  clutter_container_add (CLUTTER_CONTAINER (stage), CLUTTER_ACTOR (entry), NULL);
  g_signal_connect (entry, "status-changed", G_CALLBACK (status_changed_cb), NULL);

  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
