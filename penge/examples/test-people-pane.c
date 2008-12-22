#include <clutter/clutter.h>
#include <mojito-client/mojito-client.h>

#include "penge-people-pane.h"

typedef struct
{
  ClutterActor *stage;
  ClutterActor *pane;
  MojitoClient *client;
  MojitoClientView *view;
} PeoplePaneTestApp;

static void
client_open_view_cb (MojitoClient       *client, 
                     MojitoClientView   *view,
                     gpointer            userdata)
{
  PeoplePaneTestApp *app = (PeoplePaneTestApp *)userdata;
  app->view = view;
  mojito_client_view_start (view);
  app->pane = g_object_new (PENGE_TYPE_PEOPLE_PANE,
                            "view",
                            view,
                            NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (app->stage),
                               app->pane);
  clutter_actor_show_all (app->stage);
}

static void
client_get_sources_cb (MojitoClient *client,
                       GList        *sources,
                       gpointer      userdata)
{
  GList *filtered_sources = NULL;
  GList *l;

  for (l = sources; l; l = l->next)
  {
    if (!g_str_equal (l->data, "dummy"))
    {
      filtered_sources = g_list_append (filtered_sources, l->data);
    }
  }

  mojito_client_open_view (client, 
                           filtered_sources, 
                           6, 
                           client_open_view_cb, 
                           userdata);

  g_list_free (filtered_sources);
}

int
main (int    argc,
      char **argv)
{
  PeoplePaneTestApp *app;

  clutter_init (&argc, &argv);

  app = g_new0 (PeoplePaneTestApp, 1);

  app->stage = clutter_stage_get_default ();
  clutter_actor_set_size (app->stage, 700, 700);
  app->client = mojito_client_new ();
  mojito_client_get_sources (app->client, client_get_sources_cb, app);
  clutter_main ();

  g_free (app);
}
