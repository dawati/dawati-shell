#include <gtk/gtk.h>
#include <gconnman/gconnman.h>
#include <carrick/carrick-list.h>
#include <carrick/carrick-service-item.h>

void
add_services (gpointer data,
              gpointer user_data)
{
  GtkWidget *item;

  item = carrick_service_item_new (CM_SERVICE (data));
  carrick_list_add_item (CARRICK_LIST (user_data),
                         item);
}

int
main (int argc,
      char **argv)
{
  GtkWidget *window;
  GtkWidget *service_list;
  CmManager *manager;
  GError *error;
  GList *services;
  GtkSettings *settings;

  gtk_init (&argc, &argv);

  settings = gtk_settings_get_default ();
  gtk_settings_set_string_property (settings,
                                    "gtk-theme-name",
                                    "Moblin-Netbook",
                                    NULL);

  manager = cm_manager_new (&error);
  if (error)
  {
    g_debug ("Oh dear, management error: %s",
             error->message);
    g_clear_error (&error);
    return -1;
  }

  services = cm_manager_get_services (manager);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window,
                    "delete-event",
                    (GCallback) gtk_main_quit,
                    NULL);
  service_list = carrick_list_new ();
  gtk_container_add ((GtkContainer *)window,
                     service_list);

  g_list_foreach (services,
                  add_services,
                  (gpointer) service_list);

  gtk_widget_show_all (window);

  gtk_main ();

  g_list_free (services);
}
