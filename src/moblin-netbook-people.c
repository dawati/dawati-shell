/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 * Derived from status panel, author: Emmanuele Bassi <ebassi@linux.intel.com>
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
#include <mojito-client/mojito-client.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>


#include <nbtk/nbtk.h>

#include "moblin-netbook-people.h"
#include "mnb-drop-down.h"
#include "moblin-netbook.h"
#include "mnb-entry.h"


#include <anerley/anerley-tp-feed.h>
#include <anerley/anerley-item.h>
#include <anerley/anerley-tile-view.h>
#include <anerley/anerley-tile.h>
#include <anerley/anerley-aggregate-tp-feed.h>

#define TIMEOUT 250

static guint filter_timeout_id = 0;
static AnerleyFeedModel *model = NULL;
static NbtkWidget *drop_down = NULL;

static gboolean
_filter_timeout_cb (gpointer userdata)
{
  MnbEntry *entry = (MnbEntry *)userdata;
  anerley_feed_model_set_filter_text (model,
                                      mnb_entry_get_text (entry));
  return FALSE;
}

static void
_entry_text_changed_cb (MnbEntry *entry,
                        gpointer  userdata)
{
  if (filter_timeout_id > 0)
    g_source_remove (filter_timeout_id);

  filter_timeout_id = g_timeout_add_full (G_PRIORITY_LOW,
                                          TIMEOUT,
                                          _filter_timeout_cb,
                                          entry,
                                          NULL);
}

static void
_view_item_activated_cb (AnerleyTileView *view,
                         AnerleyItem     *item,
                         gpointer         userdata)
{
  MnbDropDown *drop_down = (MnbDropDown *)userdata;

  anerley_item_activate (item);
  clutter_actor_hide ((ClutterActor *)drop_down);
}

static void
dropdown_show_cb (MnbDropDown  *dropdown,
                  ClutterActor *filter_entry)
{
  /* give focus to the actor */
  clutter_actor_grab_key_focus (filter_entry);
}

static void
first_drop_down_cb (MnbDropDown  *dropdown,
                    ClutterActor *view)
{
  nbtk_icon_view_set_model (NBTK_ICON_VIEW (view), (ClutterModel *)model);
  g_signal_handlers_disconnect_by_func (dropdown, first_drop_down_cb, view);
}

static void
dropdown_hide_cb (MnbDropDown  *dropdown,
                  ClutterActor *filter_entry)
{
  /* Reset search. */
  mnb_entry_set_text (MNB_ENTRY (filter_entry), "");
}

#define ICON_SIZE 48

static gboolean
_enter_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      "hover");

  return FALSE;
}

static gboolean
_leave_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      NULL);

  return FALSE;
}

static gboolean
_no_people_tile_button_press_event_cb (ClutterActor *actor,
                                       ClutterEvent *event,
                                       gpointer      userdata)
{
  GAppInfo *app_info = (GAppInfo *)userdata;
  GError *error = NULL;
  GAppLaunchContext *context;

  context = G_APP_LAUNCH_CONTEXT (gdk_app_launch_context_new ());

  if (g_app_info_launch (app_info, NULL, context, &error))
  {
    clutter_actor_hide ((ClutterActor *)drop_down);
  }

  if (error)
  {
    g_warning (G_STRLOC ": Error launching application): %s",
                 error->message);
    g_clear_error (&error);
  }

  g_object_unref (context);
  return TRUE;
}

static ClutterActor *
_make_empty_people_tile (gint         width)
{
  NbtkWidget *tile;
  NbtkWidget *bin;
  NbtkWidget *label;
  ClutterActor *tex;
  GtkIconTheme *icon_theme;
  GtkIconInfo *icon_info;
  GAppInfo *app_info;
  GError *error = NULL;
  GIcon *icon;
  ClutterActor *tmp_text;
  NbtkWidget *hbox;

  tile = nbtk_table_new ();
  nbtk_table_set_row_spacing (NBTK_TABLE (tile), 8);

  clutter_actor_set_width ((ClutterActor *)tile, width);
  clutter_actor_set_name ((ClutterActor *)tile,
                          "people-people-pane-no-people-tile");
  bin = nbtk_bin_new ();
  clutter_actor_set_name ((ClutterActor *)bin,
                          "people-no-people-message-bin");
  label = nbtk_label_new (_("Sorry, we can't find any people. " \
                            "Have you set up an Instant Messenger account?"));
  clutter_actor_set_name ((ClutterActor *)label,
                          "people-people-pane-main-label");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);
  nbtk_bin_set_child (NBTK_BIN (bin), (ClutterActor *)label);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (tile),
                                        (ClutterActor *)bin,
                                        0,
                                        0,
                                        "x-expand",
                                        TRUE,
                                        "y-expand",
                                        FALSE,
                                        "x-fill",
                                        TRUE,
                                        "y-fill",
                                        FALSE,
                                        "x-align",
                                        0.0,
                                        NULL);
  nbtk_bin_set_alignment (NBTK_BIN (bin), NBTK_ALIGN_LEFT, NBTK_ALIGN_CENTER);

  app_info = (GAppInfo *)g_desktop_app_info_new ("empathy-accounts.desktop");

  if (app_info)
  {
    icon_theme = gtk_icon_theme_new ();

    icon = g_app_info_get_icon (app_info);
    icon_info = gtk_icon_theme_lookup_by_gicon (icon_theme,
                                                icon,
                                                ICON_SIZE,
                                                GTK_ICON_LOOKUP_GENERIC_FALLBACK);

    tex = clutter_texture_new_from_file (gtk_icon_info_get_filename (icon_info),
                                         &error);

    hbox = nbtk_table_new ();
    clutter_actor_set_name ((ClutterActor *)hbox,
                            "people-no-people-launcher");
    nbtk_table_set_col_spacing (NBTK_TABLE (hbox), 8);
    nbtk_table_add_actor_with_properties (NBTK_TABLE (tile),
                                          (ClutterActor *)hbox,
                                          1,
                                          0,
                                          "x-expand",
                                          FALSE,
                                          "y-expand",
                                          FALSE,
                                          "x-fill",
                                          FALSE,
                                          "y-fill",
                                          FALSE,
                                          "x-align",
                                          0.0,
                                          NULL);


    if (!tex)
    {
      g_warning (G_STRLOC ": Error opening icon: %s",
                 error->message);
      g_clear_error (&error);
    } else {
      clutter_actor_set_size (tex, ICON_SIZE, ICON_SIZE);
      nbtk_table_add_actor_with_properties (NBTK_TABLE (hbox),
                                            tex,
                                            1,
                                            0,
                                            "x-expand",
                                            FALSE,
                                            "x-fill",
                                            FALSE,
                                            "y-fill",
                                            FALSE,
                                            "y-expand",
                                            FALSE,
                                            "x-align",
                                            0.0,
                                            "y-align",
                                            0.5,
                                            NULL);
    }

    label = nbtk_label_new (g_app_info_get_description (app_info));
    clutter_actor_set_name ((ClutterActor *)label, "people-no-people-description");
    nbtk_table_add_actor_with_properties (NBTK_TABLE (hbox),
                                          (ClutterActor *)label,
                                          1,
                                          1,
                                          "x-expand",
                                          TRUE,
                                          "x-fill",
                                          FALSE,
                                          "y-expand",
                                          FALSE,
                                          "y-fill",
                                          FALSE,
                                          "x-align",
                                          0.0,
                                          "y-align",
                                          0.5,
                                          NULL);

    g_signal_connect (hbox,
                      "button-press-event",
                      (GCallback)_no_people_tile_button_press_event_cb,
                      app_info);

    g_signal_connect (hbox,
                      "enter-event",
                      (GCallback)_enter_event_cb,
                      NULL);
    g_signal_connect (hbox,
                      "leave-event",
                      (GCallback)_leave_event_cb,
                      NULL);
    clutter_actor_set_reactive ((ClutterActor *)hbox, TRUE);
  }

  return (ClutterActor *)tile;
}

static void
_model_bulk_changed_end_cb (AnerleyFeedModel *model,
                            gpointer          userdata)
{
  ClutterActor *no_people_tile = (ClutterActor *)userdata;

  if (clutter_model_get_first_iter ((ClutterModel *)model))
  {
    clutter_actor_hide (no_people_tile);
  } else {
    clutter_actor_show (no_people_tile);
  }
}

ClutterActor *
make_people_panel (MutterPlugin *plugin,
                   gint          width)
{
  NbtkWidget *vbox, *hbox, *label, *entry, *bin, *button;
  NbtkWidget *scroll_view;
  NbtkWidget *tile_view;
  ClutterText *text;
  guint items_list_width = 0, items_list_height = 0;
  MissionControl *mc;
  AnerleyFeed *feed;
  DBusGConnection *conn;
  ClutterActor *no_people_tile = NULL;

  drop_down = (NbtkWidget *)mnb_drop_down_new ();

  vbox = nbtk_table_new ();
  clutter_actor_set_size ((ClutterActor *)vbox, width, 400);
  nbtk_table_set_col_spacing (NBTK_TABLE (vbox), 12);
  nbtk_table_set_row_spacing (NBTK_TABLE (vbox), 6);
  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), CLUTTER_ACTOR (vbox));
  clutter_actor_set_name (CLUTTER_ACTOR (vbox), "people-vbox");

  hbox = nbtk_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (hbox), "people-search");
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

  label = nbtk_label_new (_("People"));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "people-search-label");
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
  clutter_actor_set_name (CLUTTER_ACTOR (entry), "people-search-entry");
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
  g_signal_connect (entry,
                    "text-changed",
                    (GCallback)_entry_text_changed_cb,
                    NULL);

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
  mc = mission_control_new (conn);
  feed = anerley_aggregate_tp_feed_new (mc);
  model = (AnerleyFeedModel *)anerley_feed_model_new (feed);
  tile_view = anerley_tile_view_new (NULL);
  scroll_view = nbtk_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll_view),
                               (ClutterActor *)tile_view);
  g_signal_connect (tile_view,
                    "item-activated",
                    (GCallback)_view_item_activated_cb,
                    drop_down);

  g_signal_connect (drop_down, "show-completed",
                    G_CALLBACK (dropdown_show_cb),
                    entry);
  g_signal_connect (drop_down, "show-completed",
                    G_CALLBACK (first_drop_down_cb),
                    tile_view);
  g_signal_connect (drop_down, "hide-completed",
                    G_CALLBACK (dropdown_hide_cb),
                    entry);

  nbtk_table_add_actor_with_properties (NBTK_TABLE (vbox),
                                        (ClutterActor *)scroll_view,
                                        1,
                                        0,
                                        "x-fill",
                                        TRUE,
                                        "x-expand",
                                        TRUE,
                                        "y-expand",
                                        TRUE,
                                        "y-fill",
                                        TRUE,
                                        NULL);

  no_people_tile = 
    _make_empty_people_tile (clutter_actor_get_width ((ClutterActor *)scroll_view));

  nbtk_table_add_actor_with_properties (NBTK_TABLE (vbox),
                                        (ClutterActor *)no_people_tile,
                                        1,
                                        0,
                                        "x-fill",
                                        TRUE,
                                        "x-expand",
                                        TRUE,
                                        "y-expand",
                                        FALSE,
                                        "y-fill",
                                        FALSE,
                                        "y-align",
                                        0.0,
                                        NULL);
  g_signal_connect (model,
                    "bulk-change-end",
                    (GCallback)_model_bulk_changed_end_cb,
                    no_people_tile);
  clutter_actor_show_all ((ClutterActor *)vbox);

  return (ClutterActor *)drop_down;
}
