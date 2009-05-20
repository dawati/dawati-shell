
#include <stdio.h>
#include <stdlib.h>

#include <clutter/clutter.h>
#include <gtk/gtk.h>
#include <nbtk/nbtk.h>
#include "moblin-netbook.h"
#include "moblin-netbook-launcher.h"

GType moblin_netbook_plugin_get_type (void);
gboolean hide_panel (MutterPlugin *plugin);

GType
moblin_netbook_plugin_get_type (void)
{
  return 0;
}

gboolean
hide_panel (MutterPlugin *plugin)
{
  g_message ("'hide_panel()' called");
  return FALSE;
}

int
main (int argc, char *argv[])
{
  ClutterActor *stage;
  ClutterActor *launcher;

  gtk_init (&argc, &argv);
  clutter_init (&argc, &argv);

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             "../data/theme/mutter-moblin.css", NULL);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 800, 600);

  launcher = moblin_netbook_launcher_panel_new (NULL, 800, 600, NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), launcher);

  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
