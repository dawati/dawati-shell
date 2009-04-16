
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

static gboolean
text_key_press_cb (ClutterActor    *actor,
                   ClutterKeyEvent *event,
                   gpointer         user_data)
{
  printf ("%s() '%x'\n", __FUNCTION__, event->keyval);
  return FALSE;
}

static gboolean
entry_key_press_cb (ClutterActor    *actor,
                    ClutterKeyEvent *event,
                    gpointer         user_data)
{
  printf ("%s() '%x'\n", __FUNCTION__, event->keyval);
  return FALSE;
}

int
main (int argc, char *argv[])
{
  NbtkWidget *entry, *inner_entry;
  ClutterActor *stage, *text;

  clutter_init (&argc, &argv);

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             "../data/theme/mutter-moblin.css", NULL);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 400, 200);

  entry = mnb_entry_new ("Foo");

  clutter_actor_set_widthu (CLUTTER_ACTOR (entry), CLUTTER_UNITS_FROM_DEVICE (200));
  clutter_actor_set_position (CLUTTER_ACTOR (entry), 50, 50);
  clutter_container_add (CLUTTER_CONTAINER (stage), CLUTTER_ACTOR (entry), NULL);

  g_signal_connect (entry, "button-clicked", G_CALLBACK (button_clicked_cb), NULL);

  inner_entry = mnb_entry_get_nbtk_entry (MNB_ENTRY (entry));
  g_signal_connect (inner_entry, "key-press-event", G_CALLBACK (entry_key_press_cb), NULL);

  text = nbtk_entry_get_clutter_text (NBTK_ENTRY (inner_entry));
  g_signal_connect (text, "key-press-event", G_CALLBACK (text_key_press_cb), NULL);

  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
