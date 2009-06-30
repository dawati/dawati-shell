
#include <stdio.h>
#include <stdlib.h>

#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include <moblin-panel/mpl-entry.h>

static void
button_clicked_cb (MplEntry *entry,
                   gpointer  user_data)
{
  printf ("%s() %s\n", __FUNCTION__, mpl_entry_get_text (entry));
}

static void
keynav_cb (ClutterActor *actor,
           guint         keyval,
           gpointer      user_data)
{
  switch (keyval)
    {
      case CLUTTER_Return:
        printf ("%s() return\n", __FUNCTION__);
        break;
      case CLUTTER_Left:
        printf ("%s() left\n", __FUNCTION__);
        break;
      case CLUTTER_Up:
        printf ("%s() up\n", __FUNCTION__);
        break;
      case CLUTTER_Right:
        printf ("%s() right\n", __FUNCTION__);
        break;
      case CLUTTER_Down:
        printf ("%s() down\n", __FUNCTION__);
        break;
    }
}

int
main (int argc, char *argv[])
{
  NbtkWidget *entry;
  ClutterActor *stage;

  clutter_init (&argc, &argv);

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             "../theme/theme.css", NULL);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 400, 200);

  entry = mpl_entry_new ("Foo");

  clutter_actor_set_width (CLUTTER_ACTOR (entry), 200);
  clutter_actor_set_position (CLUTTER_ACTOR (entry), 50, 50);
  clutter_container_add (CLUTTER_CONTAINER (stage), CLUTTER_ACTOR (entry), NULL);

  g_signal_connect (entry, "button-clicked", G_CALLBACK (button_clicked_cb), NULL);
  g_signal_connect (entry, "keynav-event", G_CALLBACK (keynav_cb), NULL);

  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
