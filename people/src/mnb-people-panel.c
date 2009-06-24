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

#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>


#include <anerley/anerley-tp-feed.h>
#include <anerley/anerley-item.h>
#include <anerley/anerley-tile-view.h>
#include <anerley/anerley-tile.h>
#include <anerley/anerley-aggregate-tp-feed.h>
#include <anerley/anerley-ebook-feed.h>
#include <anerley/anerley-tp-item.h>
#include <anerley/anerley-econtact-item.h>

#include <libebook/e-book.h>

#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-entry.h>

#include "mnb-people-panel.h"

G_DEFINE_TYPE (MnbPeoplePanel, mnb_people_panel, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_PEOPLE_PANEL, MnbPeoplePanelPrivate))

#define TIMEOUT 250

typedef struct _MnbPeoplePanelPrivate MnbPeoplePanelPrivate;

struct _MnbPeoplePanelPrivate {
  guint filter_timeout_id;
  AnerleyFeedModel *model;
  ClutterActor *tex;
  NbtkWidget *entry;
  GAppInfo *app_info;
  GtkIconTheme *icon_theme;
  NbtkWidget *primary_button;
  NbtkWidget *secondary_button;
  NbtkWidget *tile_view;
  MplPanelClient *panel_client;
  NbtkWidget *side_pane;
};

static void
mnb_people_panel_dispose (GObject *object)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (object);

  if (priv->panel_client)
  {
    g_object_unref (priv->panel_client);
    priv->panel_client = NULL;
  }

  G_OBJECT_CLASS (mnb_people_panel_parent_class)->dispose (object);
}

static void
mnb_people_panel_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_people_panel_parent_class)->finalize (object);
}

static void
mnb_people_panel_class_init (MnbPeoplePanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbPeoplePanelPrivate));

  object_class->dispose = mnb_people_panel_dispose;
  object_class->finalize = mnb_people_panel_finalize;
}


static gboolean
_filter_timeout_cb (gpointer userdata)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);

  anerley_feed_model_set_filter_text (priv->model,
                                      mpl_entry_get_text (MPL_ENTRY (priv->entry)));
  return FALSE;
}

static void
_entry_text_changed_cb (MplEntry *entry,
                        gpointer  userdata)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);

  if (priv->filter_timeout_id > 0)
    g_source_remove (priv->filter_timeout_id);

  priv->filter_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT,
                                                TIMEOUT,
                                                _filter_timeout_cb,
                                                userdata,
                                                NULL);
}

static void
_update_buttons (MnbPeoplePanel *people_panel)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (people_panel);
  gchar *msg;
  AnerleyItem *item;

  item =
    anerley_tile_view_get_selected_item ((AnerleyTileView *)priv->tile_view);

  if (item)
  {
    clutter_actor_show ((ClutterActor *)priv->primary_button);
  } else {
    clutter_actor_hide ((ClutterActor *)priv->primary_button);
    clutter_actor_hide ((ClutterActor *)priv->secondary_button);
    return;
  }

  if (ANERLEY_IS_ECONTACT_ITEM (item))
  {
    msg = g_strdup_printf (_("Email %s"),
                           anerley_item_get_display_name (item));
    nbtk_button_set_label (NBTK_BUTTON (priv->primary_button),
                           msg);
    g_free (msg);
    msg = g_strdup_printf (_("Edit %s"),
                           anerley_item_get_display_name (item));
    nbtk_button_set_label (NBTK_BUTTON (priv->secondary_button),
                           msg);
    g_free (msg);
    clutter_actor_show ((ClutterActor *)priv->secondary_button);
  } else if (ANERLEY_IS_TP_ITEM (item)) {
    msg = g_strdup_printf (_("IM %s"),
                           anerley_item_get_display_name (item));
    nbtk_button_set_label (NBTK_BUTTON (priv->primary_button),
                           msg);
    g_free (msg);
    clutter_actor_hide ((ClutterActor *)priv->secondary_button);
  } else {
    g_debug (G_STRLOC ": Unknown item type?");
  }
}

static void
dropdown_show_cb (MplPanelClient *client,
                  gpointer        userdata)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);

  /* give focus to the actor */
  clutter_actor_grab_key_focus ((ClutterActor *)priv->entry);
}


static void
dropdown_hide_cb (MplPanelClient *client,
                  gpointer        userdata)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);

  /* Reset search. */
  mpl_entry_set_text (MPL_ENTRY (priv->entry), "");
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
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);
  GError *error = NULL;
  GAppLaunchContext *context;

  context = G_APP_LAUNCH_CONTEXT (gdk_app_launch_context_new ());

  if (g_app_info_launch (priv->app_info, NULL, context, &error))
  {
    if (priv->panel_client)
      mpl_panel_client_request_hide (priv->panel_client);
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

static void
_update_fallback_icon (MnbPeoplePanel *people_panel)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (people_panel);
  GError *error = NULL;
  GtkIconInfo *icon_info;
  GIcon *icon;

  icon = g_app_info_get_icon (priv->app_info);
  icon_info = gtk_icon_theme_lookup_by_gicon (priv->icon_theme,
                                              icon,
                                              ICON_SIZE,
                                              GTK_ICON_LOOKUP_GENERIC_FALLBACK);

  clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->tex),
                                 gtk_icon_info_get_filename (icon_info),
                                 &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error opening icon: %s",
               error->message);
    g_clear_error (&error);
  }
}

static void
_icon_theme_changed_cb (GtkIconTheme *icon_theme,
                        gpointer      userdata)
{
  _update_fallback_icon ((MnbPeoplePanel *)userdata);
}

static ClutterActor *
_make_empty_people_tile (MnbPeoplePanel *people_panel,
                         gint            width)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (people_panel);

  NbtkWidget *tile;
  NbtkWidget *bin;
  NbtkWidget *label;
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
                            "Have you set up a Messenger account?"));
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

  priv->app_info = (GAppInfo *)g_desktop_app_info_new ("empathy-accounts.desktop");

  if (priv->app_info)
  {
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


    priv->tex = clutter_texture_new ();
    clutter_actor_set_size (priv->tex, ICON_SIZE, ICON_SIZE);
    nbtk_table_add_actor_with_properties (NBTK_TABLE (hbox),
                                          priv->tex,
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

    label = nbtk_label_new (g_app_info_get_description (priv->app_info));
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

    priv->icon_theme = gtk_icon_theme_get_default ();

    /* Listen for the theme change */
    g_signal_connect (priv->icon_theme,
                      "changed",
                      (GCallback)_icon_theme_changed_cb,
                      people_panel);

    _update_fallback_icon (people_panel);

    g_signal_connect (hbox,
                      "button-press-event",
                      (GCallback)_no_people_tile_button_press_event_cb,
                      people_panel);

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

static void
_primary_button_clicked_cb (NbtkButton *button,
                            gpointer    userdata)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);
  AnerleyItem *item;

  item =
    anerley_tile_view_get_selected_item ((AnerleyTileView *)priv->tile_view);

  if (item)
  {
    anerley_item_activate (item);
  }
}

static void
_secondary_button_clicked_cb (NbtkButton *button,
                              gpointer    userdata)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);
  AnerleyItem *item;
  gchar *command_line;
  const gchar *uid;

  item =
    anerley_tile_view_get_selected_item ((AnerleyTileView *)priv->tile_view);

  if (item)
  {
    uid = anerley_econtact_item_get_uid ((AnerleyEContactItem *)item);

    if (priv->panel_client)
    {
      command_line = g_strdup_printf ("contacts --uid %s",
                                    uid);
      if (!mpl_panel_client_launch_application (priv->panel_client,
                                                command_line,
                                                FALSE,
                                                -2))
      {
        g_warning (G_STRLOC ": Error launching contacts for uid: %s",
                   uid);
        g_free (command_line);
      } else {
        g_free (command_line);

        if (priv->panel_client)
          mpl_panel_client_request_hide (priv->panel_client);
      }
    }
  }
}

static void
_tile_view_selection_changed_cb (AnerleyTileView *view,
                                 gpointer         userdata)
{
  _update_buttons ((MnbPeoplePanel *)userdata);
}

static void
_tile_view_item_activated_cb (AnerleyTileView *view,
                              AnerleyItem     *item,
                              gpointer         userdata)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);

  anerley_item_activate (item);

  if (priv->panel_client)
    mpl_panel_client_request_hide (priv->panel_client);
}

static void
mnb_people_panel_init (MnbPeoplePanel *self)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);
  NbtkWidget *hbox, *label;
  NbtkWidget *scroll_view;
  MissionControl *mc;
  AnerleyFeed *feed, *tp_feed, *ebook_feed;
  DBusGConnection *conn;
  ClutterActor *no_people_tile = NULL;
  EBook *book;
  GError *error = NULL;
  ClutterActor *tmp_text;
  NbtkWidget *scroll_bin;

  nbtk_table_set_col_spacing (NBTK_TABLE (self), 4);
  nbtk_table_set_row_spacing (NBTK_TABLE (self), 6);
  clutter_actor_set_name (CLUTTER_ACTOR (self), "people-vbox");

  hbox = nbtk_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (hbox), "people-search");
  nbtk_table_set_col_spacing (NBTK_TABLE (hbox), 20);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
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

  priv->entry = mpl_entry_new (_("Search"));
  clutter_actor_set_name (CLUTTER_ACTOR (priv->entry), "people-search-entry");
  clutter_actor_set_width (CLUTTER_ACTOR (priv->entry), 600);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (hbox),
                                        CLUTTER_ACTOR (priv->entry),
                                        0, 1,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);
  g_signal_connect (priv->entry,
                    "text-changed",
                    (GCallback)_entry_text_changed_cb,
                    self);

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
  mc = mission_control_new (conn);

  feed = anerley_aggregate_feed_new ();

  tp_feed = anerley_aggregate_tp_feed_new (mc);
  anerley_aggregate_feed_add_feed (ANERLEY_AGGREGATE_FEED (feed),
                                   tp_feed);

  book = e_book_new_default_addressbook (&error);

  if (error)
  {
    g_warning (G_STRLOC ": Error getting default addressbook: %s",
               error->message);
    g_clear_error (&error);
  } else {
    ebook_feed = anerley_ebook_feed_new (book);
    anerley_aggregate_feed_add_feed (ANERLEY_AGGREGATE_FEED (feed),
                                     ebook_feed);
  }

  priv->model = (AnerleyFeedModel *)anerley_feed_model_new (feed);
  priv->tile_view = anerley_tile_view_new (priv->model);
  scroll_view = nbtk_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll_view),
                               (ClutterActor *)priv->tile_view);

  /* Use a table here rather than a bin since this give significantly better
   * scrolling peformance
   */
  scroll_bin = nbtk_table_new ();
  nbtk_table_add_actor (NBTK_TABLE (scroll_bin),
                        (ClutterActor *)scroll_view,
                        0,
                        0);
  clutter_actor_set_name ((ClutterActor *)scroll_bin, "people-scroll-bin");

  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        (ClutterActor *)scroll_bin,
                                        1,
                                        0,
                                        "x-fill",
                                        TRUE,
                                        "x-expand",
                                        TRUE,
                                        "y-expand",
                                        FALSE,
                                        "y-fill",
                                        TRUE,
                                        "row-span",
                                        1,
                                        NULL);
  no_people_tile =
    _make_empty_people_tile (self,
                             clutter_actor_get_width ((ClutterActor *)scroll_view));

  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
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
                                        "row-span",
                                        1,
                                        NULL);
  g_signal_connect (priv->model,
                    "bulk-change-end",
                    (GCallback)_model_bulk_changed_end_cb,
                    no_people_tile);
  clutter_actor_show_all ((ClutterActor *)self);

  priv->side_pane = nbtk_table_new ();
  clutter_actor_set_width ((ClutterActor *)priv->side_pane, 184);
  clutter_actor_set_name ((ClutterActor *)priv->side_pane, "people-sidepane");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        (ClutterActor *)priv->side_pane,
                                        1,
                                        1,
                                        "x-fill",
                                        FALSE,
                                        "y-fill",
                                        FALSE,
                                        "x-expand",
                                        FALSE,
                                        "y-expand",
                                        TRUE,
                                        "x-align",
                                        0.0,
                                        "y-align",
                                        0.0,
                                        NULL);

  priv->primary_button = nbtk_button_new_with_label ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->primary_button), "people-primary-action");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->side_pane),
                                        (ClutterActor *)priv->primary_button,
                                        0,
                                        0,
                                        "x-fill",
                                        TRUE,
                                        "y-fill",
                                        FALSE,
                                        "x-expand",
                                        TRUE,
                                        "y-expand",
                                        FALSE,
                                        "x-align",
                                        0.0,
                                        "y-align",
                                        0.0,
                                        NULL);

  priv->secondary_button = nbtk_button_new_with_label ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->secondary_button), "people-secondary-action");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->side_pane),
                                        (ClutterActor *)priv->secondary_button,
                                        1,
                                        0,
                                        "x-fill",
                                        TRUE,
                                        "y-fill",
                                        FALSE,
                                        "x-expand",
                                        TRUE,
                                        "y-expand",
                                        TRUE,
                                        "x-align",
                                        0.0,
                                        "y-align",
                                        0.0,
                                        NULL);

  tmp_text =
    nbtk_bin_get_child (NBTK_BIN (priv->primary_button));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);

  tmp_text =
    nbtk_bin_get_child (NBTK_BIN (priv->secondary_button));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);

  _update_buttons (self);

  g_signal_connect (priv->primary_button,
                    "clicked",
                    (GCallback)_primary_button_clicked_cb,
                    self);

  g_signal_connect (priv->secondary_button,
                    "clicked",
                    (GCallback)_secondary_button_clicked_cb,
                    self);

  g_signal_connect_after (priv->tile_view,
                          "selection-changed",
                          (GCallback)_tile_view_selection_changed_cb,
                          self);

  g_signal_connect (priv->tile_view,
                    "item-activated",
                    (GCallback)_tile_view_item_activated_cb,
                    self);
}

NbtkWidget *
mnb_people_panel_new (void)
{
  return g_object_new (MNB_TYPE_PEOPLE_PANEL, NULL);
}

void
mnb_people_panel_set_panel_client (MnbPeoplePanel *people_panel,
                                   MplPanelClient *panel_client)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (people_panel);

  priv->panel_client = g_object_ref (panel_client);

  g_signal_connect (panel_client,
                    "show-end",
                    (GCallback)dropdown_show_cb,
                    people_panel);

  g_signal_connect (panel_client,
                    "hide-end",
                    (GCallback)dropdown_hide_cb,
                    people_panel);
}


