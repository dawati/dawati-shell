#include <dalston/dalston-brightness-manager.h>
#include <dalston/dalston-brightness-slider.h>

#include <gtk/gtk.h>

int
main (int    argc,
      char **argv)
{
  DalstonBrightnessManager *manager;
  GtkWidget *main_window;
  GtkWidget *slider;

  gtk_init (&argc, &argv);

  manager = dalston_brightness_manager_new ();
  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  slider = dalston_brightness_slider_new (manager);
  gtk_container_add (GTK_CONTAINER (main_window), slider);
  gtk_widget_show_all (main_window);

  dalston_brightness_manager_start_monitoring (manager);

  gtk_main ();
}
