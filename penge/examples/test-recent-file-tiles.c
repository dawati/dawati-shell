#include <clutter/clutter.h>
#include <nbtk/nbtk.h>

#include <gtk/gtk.h>
#include <penge-recent-file-tile.h>


int
main (int    argc,
      char **argv)
{
  ClutterActor *stage;
  NbtkWidget *table;
  GtkRecentManager *manager;
  GList *items;
  GList *l;
  ClutterActor *tile;
  gint count = 0;
  GtkRecentInfo *info;

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();
  table = nbtk_table_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               (ClutterActor *)table);

  manager = gtk_recent_manager_get_default ();
  items = gtk_recent_manager_get_items (manager);

  for (l = items; l; l = l->next)
  {
    info = (GtkRecentInfo *)l->data;
    tile = g_object_new (PENGE_TYPE_RECENT_FILE_TILE,
                         "uri",
                         gtk_recent_info_get_uri (info),
                         NULL);
    nbtk_table_add_actor (NBTK_TABLE (table), tile, count / 2, count % 2);
    clutter_container_child_set (CLUTTER_CONTAINER (table),
                                 tile,
                                 "keep-aspect-ratio",
                                 TRUE,
                                 NULL);
    count++;
  }

  g_list_free (items);
  clutter_actor_show_all (stage);

  clutter_main ();
}

