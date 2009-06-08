/* Cunningly borrowed from Penge */
#include <bickley/bkl.h>

#include <nbtk/nbtk.h>
#include <ahoghill/ahoghill-grid-view.h>

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
    ClutterActor *grid;

    g_thread_init (NULL);
    bkl_init ();
    clutter_init (&argc, &argv);
    gtk_init (&argc, &argv);

    srand (time (NULL));
    load_stylesheet ();

    stage = clutter_stage_get_default ();
    clutter_actor_set_size (stage, 1024, 600);

    grid = g_object_new (AHOGHILL_TYPE_GRID_VIEW, NULL);
    clutter_actor_set_size (grid, 1024, 500);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), grid);
    clutter_actor_set_position (grid, 0, 50);
    clutter_actor_show (stage);

    clutter_main ();

    return 0;
}
