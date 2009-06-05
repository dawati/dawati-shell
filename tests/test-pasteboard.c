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
static MnbClipboardStore *store = NULL;

struct _SearchClosure
{
  MnbClipboardView *view;

  gchar *filter;
};

gchar * penge_utils_format_time (GTimeVal *time_);

gchar *
penge_utils_format_time (GTimeVal *time_)
{
  GTimeVal now;
  struct tm tm_mtime;
  gchar *retval = NULL;
  GDate d1, d2;
  gint secs_diff, mins_diff, hours_diff, days_diff, months_diff, years_diff;

  g_return_val_if_fail (time_->tv_usec >= 0 && time_->tv_usec < G_USEC_PER_SEC, NULL);

  g_get_current_time (&now);

#ifdef HAVE_LOCALTIME_R
  localtime_r ((time_t *) &(time_->tv_sec), &tm_mtime);
#else
  {
    struct tm *ptm = localtime ((time_t *) &(time_->tv_sec));

    if (!ptm)
      {
        g_warning ("ptm != NULL failed");
        return NULL;
      }
    else
      memcpy ((void *) &tm_mtime, (void *) ptm, sizeof (struct tm));
  }
#endif /* HAVE_LOCALTIME_R */

  secs_diff = now.tv_sec - time_->tv_sec;
  if (secs_diff < 60)
    return g_strdup (_("Less than a minute ago"));

  mins_diff = secs_diff / 60;
  if (mins_diff < 60)
    return g_strdup (_("A few minutes ago"));

  hours_diff = mins_diff / 60;
  if (hours_diff < 3)
    return g_strdup (_("A couple of hours ago"));

  g_date_set_time_t (&d1, now.tv_sec);
  g_date_set_time_t (&d2, time_->tv_sec);
  days_diff = g_date_get_julian (&d1) - g_date_get_julian (&d2);

  if (days_diff == 0)
    return g_strdup (_("Earlier today"));

  if (days_diff == 1)
    return g_strdup (_("Yesterday"));

  if (days_diff < 7)
    {
      const gchar *format = NULL;
      gchar *locale_format = NULL;
      gchar buf[256];

      format = _("On %A"); /* day of the week */
      locale_format = g_locale_from_utf8 (format, -1, NULL, NULL, NULL);

      if (strftime (buf, sizeof (buf), locale_format, &tm_mtime) != 0)
        retval = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
      else
        retval = g_strdup (_("Unknown"));

      g_free (locale_format);
      return retval;
    }

  if (days_diff < 14)
    return g_strdup (_("Last week"));

  if (days_diff < 21)
    return g_strdup (_("A couple of weeks ago"));

  months_diff = g_date_get_month (&d1) - g_date_get_month (&d2);
  years_diff = g_date_get_year (&d1) - g_date_get_year (&d2);

  if (years_diff == 0 && months_diff == 0)
    return g_strdup (_("This month"));

  if ((years_diff == 0 && months_diff == 1) ||
      (years_diff == 1 && months_diff == -11)) /* Now Jan., last used in Dec. */
    return g_strdup (_("Last month"));

  if (years_diff == 0)
    return g_strdup (_("This year"));

  if (years_diff == 1)
    return g_strdup (_("Last year"));

  return g_strdup (_("Ages ago"));
}

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
  if (current_selection == NULL || *current_selection == '\0')
    nbtk_label_set_text (label, _("the current selection to pasteboard"));
  else
    nbtk_label_set_text (label, current_selection);
}

static void
on_selection_copy_clicked (NbtkButton *button,
                           gpointer    dummy G_GNUC_UNUSED)
{
  g_assert (store != NULL);

  mnb_clipboard_store_save_selection (store);
}

int
main (int argc, char *argv[])
{
  ClutterActor *stage;
  NbtkWidget *vbox, *hbox, *label, *entry, *bin, *button;
  ClutterActor *view, *viewport, *scroll;
  ClutterText *text;
  gfloat items_list_width = 0, items_list_height = 0;

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
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll), viewport);
  clutter_actor_set_size (scroll, 650, 300);

  bin = NBTK_WIDGET (nbtk_bin_new ());
  clutter_actor_set_name (CLUTTER_ACTOR (bin), "pasteboard-items-list");
  clutter_container_add_actor (CLUTTER_CONTAINER (bin), scroll);
  nbtk_table_add_actor_full (NBTK_TABLE (vbox), CLUTTER_ACTOR (bin),
                             1, 0, 1, 1,
                             NBTK_X_EXPAND | NBTK_X_FILL |
                             NBTK_Y_EXPAND | NBTK_Y_FILL,
                             0., 0.);

  clutter_actor_get_size (CLUTTER_ACTOR (bin),
                          &items_list_width,
                          &items_list_height);

  clutter_actor_set_width (view, items_list_width - 50);

  g_debug ("%s: view size: %u, %u, bin size: %u, %u\n",
           G_STRLOC,
           clutter_actor_get_width (view),
           clutter_actor_get_height (view),
           items_list_width,
           items_list_height);

  /* side controls */
  bin = nbtk_table_new ();
  nbtk_table_set_row_spacing (NBTK_TABLE (bin), 12);
  clutter_actor_set_name (CLUTTER_ACTOR (bin), "pasteboard-controls");
  clutter_actor_set_size (CLUTTER_ACTOR (bin),
                          300,
                          items_list_height);
  nbtk_table_add_actor_full (NBTK_TABLE (vbox), CLUTTER_ACTOR (bin),
                             1, 1, 1, 1,
                             0,
                             0.0, 0.0);

  button = nbtk_button_new_with_label (_("Clear pasteboard"));
  nbtk_table_add_actor_full (NBTK_TABLE (bin), CLUTTER_ACTOR (button),
                             0, 0, 1, 1,
                             0,
                             0.0, 0.0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (on_clear_clicked),
                    view);

  hbox = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (hbox), 6);
  nbtk_table_add_actor_full (NBTK_TABLE (bin), CLUTTER_ACTOR (hbox),
                             1, 0, 1, 1,
                             NBTK_X_FILL | NBTK_X_EXPAND |
                             NBTK_Y_FILL | NBTK_Y_EXPAND,
                             0.0, 0.0);

  button = nbtk_button_new_with_label (_("Copy"));
  nbtk_table_add_actor_full (NBTK_TABLE (hbox), CLUTTER_ACTOR (button),
                             0, 0, 1, 1,
                             0,
                             0.0, 0.0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (on_selection_copy_clicked),
                    NULL);

  label = nbtk_label_new (_("the current selection to pasteboard"));
  text = CLUTTER_TEXT (nbtk_label_get_clutter_text (NBTK_LABEL (label)));
  clutter_text_set_single_line_mode (text, FALSE);
  clutter_text_set_line_wrap (text, TRUE);
  clutter_text_set_line_wrap_mode (text, PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (text, PANGO_ELLIPSIZE_END);
  nbtk_table_add_actor_full (NBTK_TABLE (hbox), CLUTTER_ACTOR (label),
                             0, 1, 1, 1,
                             NBTK_X_FILL | NBTK_X_EXPAND |
                             NBTK_Y_FILL | NBTK_Y_EXPAND,
                             0.0, 0.5);

  g_signal_connect (store, "selection-changed",
                    G_CALLBACK (on_selection_changed),
                    label);

  /* hook up search entry */
  g_signal_connect (entry, "button-clicked",
                    G_CALLBACK (on_search_activated), view);
  g_signal_connect (entry, "text-changed",
                    G_CALLBACK (on_search_activated), view);

  clutter_actor_show_all (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
