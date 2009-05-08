#include <bickley/bkl.h>
#include <nbtk/nbtk.h>
#include <ahoghill/ahoghill-queue-list.h>

void
load_stylesheet (void)
{
    NbtkStyle *style;
    GError *error = NULL;
    gchar *path;

    path = g_build_filename (PKG_DATADIR,
                             "theme",
                             "mutter-moblin.css",
                             NULL);

    /* register the styling */
    style = nbtk_style_get_default ();

    if (!nbtk_style_load_from_file (style,
                                    path,
                                    &error)) {
        g_warning (G_STRLOC ": Error opening style: %s",
                   error->message);
        g_clear_error (&error);
    }

    g_free (path);
}

int
main (int    argc,
      char **argv)
{
    ClutterActor *stage;
    ClutterActor *list;
    int i;

    g_thread_init (NULL);
    bkl_init ();
    clutter_init (&argc, &argv);

    load_stylesheet ();

    stage = clutter_stage_get_default ();
    clutter_actor_set_size (stage, 1024, 600);

    list = g_object_new (AHOGHILL_TYPE_QUEUE_LIST, NULL);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), list);
    clutter_actor_set_position (list, 100, 100);
    clutter_actor_set_size (list, 140, 200);

    for (i = 0; i < 10; i++) {
        ahoghill_queue_list_add_item ((AhoghillQueueList *) list, NULL);
    }

    clutter_actor_show_all (stage);

    clutter_main ();
    return 0;
}
