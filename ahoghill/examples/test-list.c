#include <bickley/bkl.h>
#include <nbtk/nbtk.h>
#include <ahoghill/ahoghill-queue-list.h>
#include <ahoghill/ahoghill-playlist.h>

void
load_stylesheet (void)
{
    NbtkStyle *style;
    GError *error = NULL;

    /* register the styling */
    style = nbtk_style_get_default ();

    if (!nbtk_style_load_from_file (style,
                                    "../theme/panel.css",
                                    &error)) {
        g_warning (G_STRLOC ": Error opening style: %s",
                   error->message);
        g_clear_error (&error);
    }
}

int
main (int    argc,
      char **argv)
{
    return 0;
}
