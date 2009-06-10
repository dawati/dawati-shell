#include <glib.h>

/*
 * These are stubs for the launching API that penge pulls in, but we do not care
 * about.
 */

gboolean
moblin_netbook_launch_application_from_info (void         *app,
                                             GList        *files,
                                             gboolean      no_chooser,
                                             gint          workspace)
{
  return FALSE;
}

gboolean
moblin_netbook_launch_application (const  gchar *path,
                                   gboolean      no_chooser,
                                   gint          workspace)
{
  return FALSE;
}

gboolean
moblin_netbook_launch_application_from_desktop_file (const  gchar *desktop,
                                                     GList        *files,
                                                     gboolean      no_chooser,
                                                     gint          workspace)
{
  return FALSE;
}

gboolean
moblin_netbook_launch_default_for_uri (const gchar *uri,
                                       gboolean     no_chooser,
                                       gint         workspace)
{
  return FALSE;
}
