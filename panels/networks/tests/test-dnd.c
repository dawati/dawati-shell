#include <carrick/carrick-list.h>
#include <gtk/gtk.h>

int
main (int argc,
      char **argv)
{
  GtkWidget *window;
  GtkWidget *item;
  GtkWidget *list;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window,
                    "delete-event",
                    (GCallback) gtk_main_quit,
                    NULL);

  list = carrick_list_new ();
  gtk_container_add (GTK_CONTAINER (window),
                     list);

  for (int i = 0; i < 10; i++)
  {
    const gchar *lbl = g_strdup_printf ("Item%i", i);
    item = gtk_button_new_with_label (lbl);
  }

  gtk_widget_set_size_request (window,
    450,
    450);
  gtk_widget_show_all (window);

  gtk_main ();
}
