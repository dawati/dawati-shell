
#include <gtk/gtk.h>
#include <meego-panel/mpl-icon-theme.h>

int
main (int     argc,
      char  **argv)
{
  GtkIconTheme  *theme;
  gchar         *icon_file;

  gtk_init (&argc, &argv);

  theme = gtk_icon_theme_get_default ();

  icon_file = mpl_icon_theme_lookup_icon_file (theme,
                                               "media-player-banshee",
                                               48);
  g_debug ("%s", icon_file);
  g_free (icon_file);

  icon_file = mpl_icon_theme_lookup_icon_file (theme,
                                               "gftp.png",
                                               48);
  g_debug ("%s", icon_file);
  g_free (icon_file);

  icon_file = mpl_icon_theme_lookup_icon_file (theme,
                                               "/usr/share/icons/skype.png",
                                               48);
  g_debug ("%s", icon_file);
  g_free (icon_file);  

  icon_file = mpl_icon_theme_lookup_icon_file (theme,
                                               "foo",
                                               48);
  g_debug ("%s", icon_file);
  g_free (icon_file);  

  return 0;
}
