#include "carrick-network-model.h"
#include "connman-marshal.h"

static void
service_to_text (GtkTreeViewColumn *column,
                 GtkCellRenderer   *cell,
                 GtkTreeModel      *model,
                 GtkTreeIter       *iter,
                 gpointer           user_data)
{
  gchar       *name, *type, *state, *markup;
  gboolean     favorite;
  const gchar *format;

  gtk_tree_model_get (model, iter,
                      CARRICK_COLUMN_NAME, &name,
                      CARRICK_COLUMN_TYPE, &type,
                      CARRICK_COLUMN_STATE, &state,
                      CARRICK_COLUMN_FAVORITE, &favorite,
                      -1);

  if (favorite)
    {
      format = "<b>%s</b> (%s) - %s";
    }
  else
    {
      format = "%s (%s) - %s";
    }

  markup = g_strdup_printf (format,
                            name,
                            type,
                            state);

  g_object_set (cell,
                "markup",
                markup,
                NULL);

  g_free (markup);
  g_free (name);
  g_free (type);
  g_free (state);
}

static GtkWidget *
create_tree (GtkTreeModel *model)
{
  GtkWidget         *tree;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *renderer;

  tree = gtk_tree_view_new ();

  column = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree),
                               column);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column,
                                   renderer,
                                   TRUE);
  gtk_tree_view_column_set_cell_data_func (column,
                                           renderer,
                                           service_to_text,
                                           NULL,
                                           NULL);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree),
                           GTK_TREE_MODEL (model));

  return tree;
}

int
main (int argc, char **argv)
{
  GtkTreeModel *model;
  GtkWidget    *tree;
  GtkWidget    *window;
  GtkWidget    *scrolled;

  if (!g_thread_supported ())
    {
      g_thread_init (NULL);
    }
  gtk_init (&argc, &argv);
  dbus_g_thread_init ();

  dbus_g_object_register_marshaller (connman_marshal_VOID__STRING_BOXED,
                                     /* return */
                                     G_TYPE_NONE,
                                     /* args */
                                     G_TYPE_STRING,
                                     G_TYPE_VALUE,
                                     /* eom */
                                     G_TYPE_INVALID);

  model = carrick_network_model_new ();
  tree = create_tree (model);
  gtk_tree_view_set_model (GTK_TREE_VIEW (tree),
                           GTK_TREE_MODEL (model));
  gtk_widget_show (tree);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled),
                     tree);
  gtk_widget_show (scrolled);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window,
                    "delete-event",
                    G_CALLBACK (gtk_main_quit),
                    NULL);
  gtk_container_add (GTK_CONTAINER (window),
                     scrolled);
  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
