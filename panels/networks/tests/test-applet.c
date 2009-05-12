#include <gtk/gtk.h>

#include <carrick/carrick-applet.h>

int
main (int    argc,
      char **argv)
{
  GtkWidget *window;
  CarrickApplet *applet;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window,
                    "delete-event",
                    (GCallback) gtk_main_quit,
                    NULL);

  applet = carrick_applet_new ();
  gtk_container_add (GTK_CONTAINER (window),
                     carrick_applet_get_pane (CARRICK_APPLET (applet)));

  gtk_widget_set_size_request (GTK_WIDGET (window),
                               1024,
                               600);
  gtk_widget_show_all (window);

  gtk_main ();
}
