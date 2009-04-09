#include <penge/penge-app-bookmark-manager.h>

#define DEVHELP_DESKTOP_URI "file:///usr/share/applications/devhelp.desktop"
#define EVOLUTION_DESKTOP_URI "file:///usr/share/applications/evolution.desktop"

int
main (int     argc,
      char  **argv)
{
  PengeAppBookmarkManager *manager;
  GList *l;
  PengeAppBookmark *bookmark;
  GMainLoop *loop;

  g_type_init ();


  loop = g_main_loop_new (NULL, TRUE);
  manager = g_object_new (PENGE_TYPE_APP_BOOKMARK_MANAGER,
                          NULL);

  penge_app_bookmark_manager_add_from_uri (manager,
                                           DEVHELP_DESKTOP_URI,
                                           NULL);

  penge_app_bookmark_manager_add_from_uri (manager,
                                           EVOLUTION_DESKTOP_URI,
                                           NULL);


  for (l = penge_app_bookmark_manager_get_bookmarks (manager); l; l = l->next)
  {
    bookmark = (PengeAppBookmark *)l->data;

    g_debug ("%s %s %s %s",
             bookmark->uri,
             bookmark->application_name,
             bookmark->icon_name,
             bookmark->app_exec);
  }

  g_main_loop_run (loop);

  return 0;
}
