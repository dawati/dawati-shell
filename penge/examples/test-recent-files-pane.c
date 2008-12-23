#include "penge-recent-files-pane.h"

typedef struct
{
  ClutterActor *stage;
  ClutterActor *pane;
} RecentFilesPaneTestApp;

int
main (int    argc,
      char **argv)
{
  RecentFilesPaneTestApp *app;

  clutter_init (&argc, &argv);

  app = g_new0 (RecentFilesPaneTestApp, 1);

  app->stage = clutter_stage_get_default ();
  clutter_actor_set_size (app->stage, 700, 700);

  app->pane = g_object_new (PENGE_TYPE_RECENT_FILES_PANE, NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (app->stage), app->pane);
  clutter_actor_show_all (app->stage);
  clutter_main ();

  g_free (app);
}
