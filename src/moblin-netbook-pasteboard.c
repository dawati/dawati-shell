/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <gtk/gtk.h>
#include <nbtk/nbtk.h>

#include <penge/penge-app-bookmark-manager.h>

#include "moblin-netbook.h"
#include "moblin-netbook-chooser.h"
#include "moblin-netbook-panel.h"
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

static MnbClipboardStore *clipboard = NULL;

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

static void
on_search_activated (MnbEntry *entry,
                     gpointer  data)
{

}

static void
on_clear_clicked (NbtkButton        *button,
                  MnbClipboardStore *store)
{
  while (clutter_model_get_n_rows (CLUTTER_MODEL (store)))
    clutter_model_remove (CLUTTER_MODEL (store), 0);
}

ClutterActor *
make_pasteboard (MutterPlugin *plugin,
                 gint          width)
{
  NbtkWidget   *vbox, *hbox, *label, *entry, *drop_down, *bin, *button;
  ClutterActor *view, *viewport, *scroll;

  drop_down = mnb_drop_down_new ();

  vbox = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (vbox), 4);
  nbtk_table_set_row_spacing (NBTK_TABLE (vbox), WIDGET_SPACING);
  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), CLUTTER_ACTOR (vbox));
  clutter_actor_set_name (CLUTTER_ACTOR (vbox), "pasteboard-vbox");

  /* Filter row. */
  hbox = nbtk_table_new ();
  nbtk_table_set_col_spacing (NBTK_TABLE (hbox), 20);
  nbtk_table_add_widget_full (NBTK_TABLE (vbox), hbox,
                              0, 0, 1, 2,
                              NBTK_X_FILL | NBTK_X_EXPAND,
                              0.0, 0.0);
  clutter_actor_set_name (CLUTTER_ACTOR (hbox), "pasteboard-search");

  label = nbtk_label_new (_("Pasteboard"));
  nbtk_table_add_widget_full (NBTK_TABLE (hbox), label,
                              0, 0, 1, 1,
                              0,
                              0., 0.5);
  clutter_actor_set_name (CLUTTER_ACTOR (label), "pasteboard-search-label");

  entry = mnb_entry_new (_("Search"));
  clutter_actor_set_width (CLUTTER_ACTOR (entry),
                           CLUTTER_UNITS_FROM_DEVICE (600));
  nbtk_table_add_widget_full (NBTK_TABLE (hbox), entry,
                              0, 1, 1, 1,
                              0,
                              0., 0.5);
  clutter_actor_set_name (CLUTTER_ACTOR (entry), "pasteboard-search-entry");
  g_signal_connect (drop_down, "show-completed",
                    G_CALLBACK (on_dropdown_show), entry);
  g_signal_connect (drop_down, "hide-completed",
                    G_CALLBACK (on_dropdown_hide), entry);

  /* the actual view */
  view = CLUTTER_ACTOR (mnb_clipboard_view_new (NULL));
  clutter_actor_set_width (view, 650);

  viewport = CLUTTER_ACTOR (nbtk_viewport_new ());
  clutter_container_add_actor (CLUTTER_CONTAINER (viewport), view);

  /* the scroll view is bigger to avoid the horizontal scroll bar */
  scroll = CLUTTER_ACTOR (nbtk_scroll_view_new ());
  clutter_actor_set_size (scroll, 680, 300);
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll), viewport);

  bin = NBTK_WIDGET (nbtk_bin_new ());
  clutter_container_add_actor (CLUTTER_CONTAINER (bin), scroll);
  clutter_actor_set_name (CLUTTER_ACTOR (bin), "pasteboard-items-list");

  nbtk_table_add_widget_full (NBTK_TABLE (vbox), bin,
                              1, 0, 1, 1,
                              0,
                              0., 0.);

  g_signal_connect (entry, "button-clicked",
                    G_CALLBACK (on_search_activated), NULL);
  g_signal_connect (entry, "text-changed",
                    G_CALLBACK (on_search_activated), NULL);

  /* side controls */
  bin = NBTK_WIDGET (nbtk_bin_new ());
  nbtk_table_add_widget_full (NBTK_TABLE (vbox), bin,
                              1, 1, 1, 1,
                              NBTK_X_FILL | NBTK_X_EXPAND,
                              0.0, 0.0);
  clutter_actor_set_name (CLUTTER_ACTOR (bin), "pasteboard-controls");

  button = nbtk_button_new_with_label (_("Clear pasteboard"));
  nbtk_bin_set_child (NBTK_BIN (bin), CLUTTER_ACTOR (button));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (on_clear_clicked),
                    mnb_clipboard_view_get_store (MNB_CLIPBOARD_VIEW (view)));

  return CLUTTER_ACTOR (drop_down);
}
