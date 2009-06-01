#include <bickley/bkl.h>
#include <nbtk/nbtk.h>
#include <ahoghill/ahoghill-queue-list.h>
#include <ahoghill/ahoghill-playlist.h>

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
    return 0;
}
