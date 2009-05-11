#include "carrick-icon-factory.h"
#include <gconnman/gconnman.h>

int
main (int    argc,
      char **argv)
{
  CarrickIconFactory *icon_factory;
  CarrickIconState state = CARRICK_ICON_NO_NETWORK;
  GtkWidget *window;
  GtkWidget *image;
  CmManager *manager;
  CmService *service;
  GError *error = NULL;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window,
                    "delete-event",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  icon_factory = carrick_icon_factory_new ();
  manager = cm_manager_new (&error);
  if (error)
  {
    g_debug ("Error initialising connman manager: %s\n",
             error->message);
    g_clear_error (&error);
    return -1;
  }
  cm_manager_refresh (manager);
  service = cm_manager_get_active_service (manager);

  g_print ("No network icon is: %s\n",
           carrick_icon_factory_path_for_state (state));
  state = carrick_icon_factory_state_for_service (NULL);
  g_print ("NULL service (%i) icon is :%s\n",
           state,
           carrick_icon_factory_path_for_state (state));

  image = carrick_icon_factory_image_for_service (icon_factory,
                                                  NULL);
  gtk_container_add (GTK_CONTAINER (window),
                     image);

  gtk_widget_set_size_request (window,
                               50,
                               50);
  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
