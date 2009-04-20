#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <glib.h>

#include <clutter/clutter.h>
#include <nbtk/nbtk.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "mnb-clipboard-item.h"
#include "mnb-clipboard-store.h"
#include "mnb-clipboard-view.h"
#include "mnb-entry.h"

typedef struct _SearchClosure   SearchClosure;

static guint search_timeout_id = 0;

struct _SearchClosure
{
  MnbClipboardView *view;

  gchar *filter;
};

static gboolean
search_timeout (gpointer data)
{
  SearchClosure *closure = data;

  mnb_clipboard_view_filter (closure->view, closure->filter);

  return FALSE;
}

static void
search_cleanup (gpointer data)
{
  SearchClosure *closure = data;

  search_timeout_id = 0;

  g_object_unref (closure->view);
  g_free (closure->filter);

  g_slice_free (SearchClosure, closure);
}

static void
on_search_activated (MnbEntry *entry,
                     gpointer  data)
{
  MnbClipboardView *view = data;
  SearchClosure *closure;

  closure = g_slice_new (SearchClosure);
  closure->view = g_object_ref (view);
  closure->filter = g_strdup (mnb_entry_get_text (entry));

  if (search_timeout_id != 0)
    {
      g_source_remove (search_timeout_id);
      search_timeout_id = 0;
    }

  search_timeout_id = g_timeout_add_full (G_PRIORITY_LOW, 250,
                                          search_timeout,
                                          closure, search_cleanup);
}

static void
on_clear_clicked (NbtkButton *button,
                  MnbClipboardView *view)
{
  MnbClipboardStore *store = mnb_clipboard_view_get_store (view);

  while (clutter_model_get_n_rows (CLUTTER_MODEL (store)))
    clutter_model_remove (CLUTTER_MODEL (store), 0);
}

static void
on_selection_changed (MnbClipboardStore *store,
                      const gchar       *current_selection,
                      NbtkLabel         *label)
{
  nbtk_label_set_text (label, current_selection);
}

int
main (int argc, char *argv[])
{
  ClutterActor *stage;
  NbtkWidget *vbox, *hbox, *label, *entry, *bin, *button;
  ClutterActor *view, *viewport, *scroll;
  ClutterText *text;
  MnbClipboardStore *store;
  guint items_list_height = 0;

  gtk_init (&argc, &argv);
  clutter_init (&argc, &argv);

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             "../data/theme/mutter-moblin.css",
                             NULL);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 1024, 600);

  vbox = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (vbox), 12);
  nbtk_table_set_row_spacing (NBTK_TABLE (vbox), 6);
  clutter_actor_set_width (CLUTTER_ACTOR (vbox), 1000);
  clutter_actor_set_name (CLUTTER_ACTOR (vbox), "pasteboard-vbox");
  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               CLUTTER_ACTOR (vbox));

  /* Filter row. */
  hbox = nbtk_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (hbox), "pasteboard-search");
  nbtk_table_set_col_spacing (NBTK_TABLE (hbox), 20);
  nbtk_table_add_actor_full (NBTK_TABLE (vbox), CLUTTER_ACTOR (hbox),
                             0, 0, 1, 2,
                             NBTK_X_FILL | NBTK_X_EXPAND,
                             0.0, 0.0);

  label = nbtk_label_new (_("Pasteboard"));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "pasteboard-search-label");
  nbtk_table_add_actor_full (NBTK_TABLE (hbox), CLUTTER_ACTOR (label),
                             0, 0, 1, 1,
                             0,
                             0., 0.5);

  entry = mnb_entry_new (_("Search"));
  clutter_actor_set_name (CLUTTER_ACTOR (entry), "pasteboard-search-entry");
  clutter_actor_set_width (CLUTTER_ACTOR (entry),
                           CLUTTER_UNITS_FROM_DEVICE (600));
  nbtk_table_add_actor_full (NBTK_TABLE (hbox), CLUTTER_ACTOR (entry),
                             0, 1, 1, 1,
                             0,
                             0., 0.5);

  /* the object proxying the Clipboard changes and storing them */
  store = mnb_clipboard_store_new ();

  /* the actual view */
  view = CLUTTER_ACTOR (mnb_clipboard_view_new (store));

  viewport = CLUTTER_ACTOR (nbtk_viewport_new ());
  clutter_container_add_actor (CLUTTER_CONTAINER (viewport), view);

  /* the scroll view is bigger to avoid the horizontal scroll bar */
  scroll = CLUTTER_ACTOR (nbtk_scroll_view_new ());
  clutter_actor_set_height (scroll, 300);
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll), viewport);

  bin = NBTK_WIDGET (nbtk_bin_new ());
  clutter_actor_set_name (CLUTTER_ACTOR (bin), "pasteboard-items-list");
  clutter_container_add_actor (CLUTTER_CONTAINER (bin), scroll);
  nbtk_table_add_actor_full (NBTK_TABLE (vbox), CLUTTER_ACTOR (bin),
                             1, 0, 1, 1,
                             NBTK_X_EXPAND | NBTK_X_FILL |
                             NBTK_Y_EXPAND | NBTK_Y_FILL,
                             0., 0.);

  items_list_height = clutter_actor_get_height (CLUTTER_ACTOR (bin));

  /* side controls */
  bin = nbtk_table_new ();
  nbtk_table_set_row_spacing (NBTK_TABLE (bin), 12);
  clutter_actor_set_name (CLUTTER_ACTOR (bin), "pasteboard-controls");
  clutter_actor_set_size (CLUTTER_ACTOR (bin), 300, items_list_height);
  nbtk_table_add_actor_full (NBTK_TABLE (vbox), CLUTTER_ACTOR (bin),
                             1, 1, 1, 1,
                             0,
                             0.0, 0.0);

  hbox = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (hbox), 6);
  nbtk_table_add_actor_full (NBTK_TABLE (bin), CLUTTER_ACTOR (hbox),
                             0, 0, 1, 1,
                             NBTK_X_FILL | NBTK_X_EXPAND,
                             0.0, 0.0);

  button = nbtk_button_new_with_label (_("Copy"));
  nbtk_table_add_actor_full (NBTK_TABLE (hbox), CLUTTER_ACTOR (button),
                             0, 0, 1, 1,
                             0,
                             0.0, 0.0);

  label = nbtk_label_new ("current selection to pasteboard");
  text = CLUTTER_TEXT (nbtk_label_get_clutter_text (NBTK_LABEL (label)));
  clutter_text_set_single_line_mode (text, FALSE);
  clutter_text_set_line_wrap (text, TRUE);
  clutter_text_set_line_wrap_mode (text, PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (text, PANGO_ELLIPSIZE_END);
  nbtk_table_add_actor_full (NBTK_TABLE (hbox), CLUTTER_ACTOR (label),
                             0, 1, 1, 1,
                             NBTK_X_FILL | NBTK_X_EXPAND,
                             0.0, 0.5);

  g_signal_connect (store, "selection-changed",
                    G_CALLBACK (on_selection_changed),
                    label);

  button = nbtk_button_new_with_label (_("Clear pasteboard"));
  nbtk_table_add_actor_full (NBTK_TABLE (bin), CLUTTER_ACTOR (button),
                             1, 0, 1, 1,
                             0,
                             0.0, 0.0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (on_clear_clicked),
                    view);

  /* hook up search entry */
  g_signal_connect (entry, "button-clicked",
                    G_CALLBACK (on_search_activated), view);
  g_signal_connect (entry, "text-changed",
                    G_CALLBACK (on_search_activated), view);

  clutter_actor_show_all (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
