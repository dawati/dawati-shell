/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Emmanuele Bassi <ebassi@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <glib/gi18n.h>

#include <gtk/gtk.h>
#include <nbtk/nbtk.h>

#include <penge/penge-app-bookmark-manager.h>

#include "moblin-netbook.h"
#include "moblin-netbook-chooser.h"
#include "moblin-netbook-pasteboard.h"
#include "mnb-clipboard-store.h"
#include "mnb-clipboard-view.h"
#include "mnb-drop-down.h"
#include "mnb-entry.h"

#define WIDGET_SPACING 5
#define ICON_SIZE 48
#define PADDING 8
#define N_COLS 4
#define LAUNCHER_WIDTH  235
#define LAUNCHER_HEIGHT 64

typedef struct _SearchClosure   SearchClosure;

static guint search_timeout_id = 0;
static MnbClipboardStore *store = NULL;

static void
on_dropdown_show (MnbDropDown  *dropdown,
                  ClutterActor *filter_entry)
{
  /* give focus to the actor */
  clutter_actor_grab_key_focus (filter_entry);
}

static void
on_dropdown_hide (MnbDropDown  *dropdown,
                  ClutterActor *filter_entry)
{
  /* Reset search. */
  mnb_entry_set_text (MNB_ENTRY (filter_entry), "");
}

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

  search_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT, 250,
                                          search_timeout,
                                          closure, search_cleanup);
}

static void
on_clear_clicked (NbtkButton *button,
                  gpointer    dummy G_GNUC_UNUSED)
{
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
    {
      gchar *text;

      text = g_strconcat (_("your selection"),
                          " \"", current_selection, "\"",
                          NULL);
      nbtk_label_set_text (label, text);

      g_free (text);
    }
}

static void
on_selection_copy_clicked (NbtkButton *button,
                           gpointer    dummy G_GNUC_UNUSED)
{
  mnb_clipboard_store_save_selection (store);
}

static void on_item_added (MnbClipboardStore    *store,
                           MnbClipboardItemType  item_type,
                           ClutterActor         *bin);

static void
on_item_added (MnbClipboardStore    *store,
               MnbClipboardItemType  item_type,
               ClutterActor         *bin)
{
  g_signal_handlers_disconnect_by_func (store,
                                        G_CALLBACK (on_item_added),
                                        bin);

  clutter_actor_destroy (bin);
}

ClutterActor *
make_pasteboard (MutterPlugin *plugin,
                 gint          width)
{
  NbtkWidget *vbox, *hbox, *label, *entry, *drop_down, *bin, *button;
  ClutterActor *view, *viewport, *scroll;
  ClutterText *text;
  gfloat items_list_width = 0, items_list_height = 0;

  drop_down = mnb_drop_down_new (plugin);

  /* the object proxying the Clipboard changes and storing them */
  store = mnb_clipboard_store_new ();

  vbox = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (vbox), 12);
  nbtk_table_set_row_spacing (NBTK_TABLE (vbox), 6);
  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), CLUTTER_ACTOR (vbox));
  clutter_actor_set_name (CLUTTER_ACTOR (vbox), "pasteboard-vbox");

  /* Filter row. */
  hbox = nbtk_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (hbox), "pasteboard-search");
  nbtk_table_set_col_spacing (NBTK_TABLE (hbox), 20);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (vbox),
                                        CLUTTER_ACTOR (hbox),
                                        0, 0,
                                        "row-span", 1,
                                        "col-span", 2,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        NULL);

  label = nbtk_label_new (_("Pasteboard"));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "pasteboard-search-label");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (hbox),
                                        CLUTTER_ACTOR (label),
                                        0, 0,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);

  entry = mnb_entry_new (_("Search"));
  clutter_actor_set_name (CLUTTER_ACTOR (entry), "pasteboard-search-entry");
  clutter_actor_set_width (CLUTTER_ACTOR (entry), 600);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (hbox),
                                        CLUTTER_ACTOR (entry),
                                        0, 1,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);
  g_signal_connect (drop_down, "show-completed",
                    G_CALLBACK (on_dropdown_show), entry);
  g_signal_connect (drop_down, "hide-completed",
                    G_CALLBACK (on_dropdown_hide), entry);

  /* bin for the the "pasteboard is empty" notice */
  bin = NBTK_WIDGET (nbtk_bin_new ());
  nbtk_widget_set_style_class_name (bin, "pasteboard-empty-bin");
  nbtk_bin_set_alignment (NBTK_BIN (bin),
                          NBTK_ALIGN_LEFT,
                          NBTK_ALIGN_CENTER);
  nbtk_bin_set_fill (NBTK_BIN (bin), TRUE, FALSE);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (vbox), CLUTTER_ACTOR (bin),
                                        1, 0,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        "row-span", 1,
                                        "col-span", 2,
                                        NULL);

  label = nbtk_label_new (_("You need to copy some text to use Pasteboard"));
  nbtk_widget_set_style_class_name (label, "pasteboard-empty-label");
  clutter_container_add_actor (CLUTTER_CONTAINER (bin),
                               CLUTTER_ACTOR (label));

  g_signal_connect (store, "item-added",
                    G_CALLBACK (on_item_added),
                    bin);

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
  nbtk_table_add_actor_with_properties (NBTK_TABLE (vbox),
                                        CLUTTER_ACTOR (bin),
                                        2, 0,
                                        "x-expand", TRUE,
                                        "y-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        NULL);

  clutter_actor_get_size (CLUTTER_ACTOR (bin),
                          &items_list_width,
                          &items_list_height);
  clutter_actor_set_width (view, items_list_width - 50);

  /* hook up the search entry to the view */
  g_signal_connect (entry, "button-clicked",
                    G_CALLBACK (on_search_activated), view);
  g_signal_connect (entry, "text-changed",
                    G_CALLBACK (on_search_activated), view);


  /* side controls */
  bin = nbtk_table_new ();
  nbtk_table_set_row_spacing (NBTK_TABLE (bin), 12);
  clutter_actor_set_name (CLUTTER_ACTOR (bin), "pasteboard-controls");
  clutter_actor_set_size (CLUTTER_ACTOR (bin), 300, items_list_height);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (vbox),
                                        CLUTTER_ACTOR (bin),
                                        2, 1,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        NULL);

  button = nbtk_button_new_with_label (_("Clear pasteboard"));
  nbtk_table_add_actor_with_properties (NBTK_TABLE (bin),
                                        CLUTTER_ACTOR (button),
                                        0, 0,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        NULL);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (on_clear_clicked),
                    NULL);

  hbox = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (hbox), 6);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (bin),
                                        CLUTTER_ACTOR (hbox),
                                        1, 0,
                                        "x-expand", TRUE,
                                        "y-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        NULL);

  button = nbtk_button_new_with_label (_("Copy"));
  nbtk_table_add_actor_with_properties (NBTK_TABLE (hbox),
                                        CLUTTER_ACTOR (button),
                                        0, 0,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        NULL);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (on_selection_copy_clicked),
                    store);

  label = nbtk_label_new (_("the current selection to pasteboard"));
  text = CLUTTER_TEXT (nbtk_label_get_clutter_text (NBTK_LABEL (label)));
  clutter_text_set_single_line_mode (text, FALSE);
  clutter_text_set_line_wrap (text, TRUE);
  clutter_text_set_line_wrap_mode (text, PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (text, PANGO_ELLIPSIZE_END);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (hbox),
                                        CLUTTER_ACTOR (label),
                                        0, 1,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", TRUE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);

  g_signal_connect (store, "selection-changed",
                    G_CALLBACK (on_selection_changed),
                    label);

  return CLUTTER_ACTOR (drop_down);
}
