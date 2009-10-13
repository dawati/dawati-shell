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
#include <anerley/anerley-tp-monitor-feed.h>
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

#define OFFLINE_BANNER _("To see your IM contacts, go online in the Status panel.")

typedef struct _MnbPeoplePanelPrivate MnbPeoplePanelPrivate;

struct _MnbPeoplePanelPrivate {
  AnerleyFeed *tp_feed;
  guint filter_timeout_id;
  AnerleyFeedModel *model;
  AnerleyFeedModel *active_model;
  ClutterActor *tex;
  NbtkWidget *entry;
  GAppInfo *app_info;
  GtkIconTheme *icon_theme;
  NbtkWidget *primary_button;
  NbtkWidget *secondary_button;
  NbtkWidget *tile_view;
  NbtkWidget *active_tile_view;
  MplPanelClient *panel_client;
  NbtkWidget *selection_pane;
  NbtkWidget *control_box;
  NbtkWidget *no_people_tile;
  NbtkWidget *selected_tile;
  NbtkWidget *nobody_selected_box;
  NbtkWidget *content_table;
  NbtkWidget *active_content_table;
  NbtkWidget *everybody_offline_tile;
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
_update_selected_item (MnbPeoplePanel *people_panel,
                       AnerleyItem    *item)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (people_panel);

  if (item)
  {
    clutter_actor_show ((ClutterActor *)priv->selected_tile);
    clutter_actor_show ((ClutterActor *)priv->primary_button);

    g_object_set (priv->selected_tile,
                  "item",
                  item,
                  NULL);
    clutter_actor_show ((ClutterActor *)priv->control_box);
    clutter_actor_hide ((ClutterActor *)priv->nobody_selected_box);
  } else {
    clutter_actor_hide ((ClutterActor *)priv->selected_tile);
    clutter_actor_hide ((ClutterActor *)priv->primary_button);
    clutter_actor_hide ((ClutterActor *)priv->secondary_button);
    clutter_actor_hide ((ClutterActor *)priv->control_box);
    clutter_actor_show ((ClutterActor *)priv->nobody_selected_box);
    return;
  }

  if (ANERLEY_IS_ECONTACT_ITEM (item))
  {
    nbtk_button_set_label (NBTK_BUTTON (priv->primary_button),
                           _("Send an email"));
    nbtk_button_set_label (NBTK_BUTTON (priv->secondary_button),
                           _("Edit details"));
    clutter_actor_show ((ClutterActor *)priv->secondary_button);

    if (!anerley_econtact_item_get_email (ANERLEY_ECONTACT_ITEM (item)))
    {
      clutter_actor_hide ((ClutterActor *)priv->primary_button);
    }
  } else if (ANERLEY_IS_TP_ITEM (item)) {
    nbtk_button_set_label (NBTK_BUTTON (priv->primary_button),
                           _("Send a message"));
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

static NbtkWidget *
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
  nbtk_bin_set_alignment (NBTK_BIN (bin), NBTK_ALIGN_START, NBTK_ALIGN_MIDDLE);

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

  return tile;
}

static NbtkWidget *
_make_everybody_offline_tile (MnbPeoplePanel *pane,
                              gint            width)
{
  NbtkWidget *tile;
  ClutterActor *tmp_text;
  NbtkWidget *label, *bin;

  tile = nbtk_table_new ();
  nbtk_table_set_row_spacing (NBTK_TABLE (tile), 8);

  clutter_actor_set_width ((ClutterActor *)tile, width);
  clutter_actor_set_name ((ClutterActor *)tile,
                          "people-pane-everybody-offline-tile");
  label = nbtk_label_new (_("Sorry, we can't find any people. " \
                            "It looks like they are all offline."));
  clutter_actor_set_name ((ClutterActor *)label,
                          "people-pane-everybody-offline-label");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);

  bin = nbtk_bin_new ();
  nbtk_bin_set_child (NBTK_BIN (bin), (ClutterActor *)label);
  nbtk_bin_set_alignment (NBTK_BIN (bin), NBTK_ALIGN_START, NBTK_ALIGN_MIDDLE);
  nbtk_bin_set_fill (NBTK_BIN (bin), FALSE, TRUE);
  clutter_actor_set_name ((ClutterActor *)bin,
                          "people-pane-everybody-offline-bin");

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
  return tile;
}


static void
_active_model_bulk_change_end_cb (AnerleyFeedModel *model,
                                  gpointer          userdata)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);
  gint number_active_people;
  gchar *tooltip;

  if (clutter_model_get_first_iter ((ClutterModel *)model))
  {
    clutter_actor_show ((ClutterActor *)priv->active_content_table);
    nbtk_table_set_row_spacing (NBTK_TABLE (priv->content_table), 6);

    if (priv->panel_client)
    {
      mpl_panel_client_request_button_style (priv->panel_client,
                                             "people-button-active");
      number_active_people = clutter_model_get_n_rows (CLUTTER_MODEL (priv->active_model));

      if (number_active_people > 1)
      {
        tooltip = g_strdup_printf (_("people - %d people are talking to you"),
                                   number_active_people);
        mpl_panel_client_request_tooltip (priv->panel_client,
                                          tooltip);
        g_free (tooltip);
      } else {
        mpl_panel_client_request_tooltip (priv->panel_client,
                                          _("people - Someone is talking to you"));
      }
    }
  } else {
    nbtk_table_set_row_spacing (NBTK_TABLE (priv->content_table), 0);
    clutter_actor_hide ((ClutterActor *)priv->active_content_table);

    if (priv->panel_client)
    {
      mpl_panel_client_request_button_style (priv->panel_client,
                                             "people-button");
      mpl_panel_client_request_tooltip (priv->panel_client,
                                        _("people"));
    }
  }

  /* Workaround for MB#6690 */
  clutter_actor_queue_relayout (CLUTTER_ACTOR (priv->content_table));
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

    if (priv->panel_client)
      mpl_panel_client_request_hide (priv->panel_client);
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
                                                command_line))
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
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);
  AnerleyItem *item;

  anerley_tile_view_clear_selected_item ((AnerleyTileView *)priv->active_tile_view);

  item =
    anerley_tile_view_get_selected_item ((AnerleyTileView *)priv->tile_view);

  _update_selected_item ((MnbPeoplePanel *)userdata,
                         item);
}

static void
_active_tile_view_selection_changed_cb (AnerleyTileView *view,
                                        gpointer         userdata)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);
  AnerleyItem *item;

  anerley_tile_view_clear_selected_item ((AnerleyTileView *)priv->tile_view);

  item = anerley_tile_view_get_selected_item ((AnerleyTileView *)priv->active_tile_view);
  _update_selected_item ((MnbPeoplePanel *)userdata,
                         item);
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
_update_placeholder_state (MnbPeoplePanel *self)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);
  AnerleyAggregateTpFeed *aggregate;

  /* There is something in the model, hide all placeholders */
  if (clutter_model_get_first_iter (CLUTTER_MODEL (priv->model)))
  {
    clutter_actor_hide ((ClutterActor *)priv->no_people_tile);
    clutter_actor_hide ((ClutterActor *)priv->everybody_offline_tile);

    /* Ensure content stuff is visible */
    clutter_actor_show ((ClutterActor *)priv->content_table);
    clutter_actor_show ((ClutterActor *)priv->selection_pane);
  } else {
    /* Hide real content stuff */
    clutter_actor_hide ((ClutterActor *)priv->content_table);
    clutter_actor_hide ((ClutterActor *)priv->selection_pane);

    aggregate = ANERLEY_AGGREGATE_TP_FEED (priv->tp_feed);
    if (anerley_aggregate_tp_feed_get_accounts_online (aggregate) == 0)
    {
      clutter_actor_show ((ClutterActor *)priv->no_people_tile);
      clutter_actor_hide ((ClutterActor *)priv->everybody_offline_tile);
    } else {
      clutter_actor_show ((ClutterActor *)priv->everybody_offline_tile);
      clutter_actor_hide ((ClutterActor *)priv->no_people_tile);
    }
  }
}

static void
_tp_feed_online_notify_cb (GObject    *object,
                           GParamSpec *pspec,
                           gpointer    userdata)
{
  _update_placeholder_state (MNB_PEOPLE_PANEL (userdata));
}

static void
_model_bulk_changed_end_cb (AnerleyFeedModel *model,
                            gpointer          userdata)
{
  _update_placeholder_state (MNB_PEOPLE_PANEL (userdata));
}

static void
mnb_people_panel_init (MnbPeoplePanel *self)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);
  NbtkWidget *hbox, *label;
  NbtkWidget *scroll_view, *scroll_bin, *bin;
  AnerleyFeed *feed, *ebook_feed, *active_feed;
  EBook *book;
  GError *error = NULL;

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

  feed = anerley_aggregate_feed_new ();

  priv->tp_feed = anerley_aggregate_tp_feed_new ();
  anerley_aggregate_feed_add_feed (ANERLEY_AGGREGATE_FEED (feed),
                                   priv->tp_feed);

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

  active_feed = anerley_tp_monitor_feed_new ((AnerleyAggregateTpFeed *)priv->tp_feed);
  priv->active_model = (AnerleyFeedModel *)anerley_feed_model_new (active_feed);
  priv->active_tile_view = anerley_tile_view_new (priv->active_model);

  priv->content_table = nbtk_table_new ();

  /* active conversations */
  priv->active_content_table = nbtk_table_new ();

  clutter_actor_hide ((ClutterActor *)priv->active_content_table);
  clutter_actor_set_name ((ClutterActor *)priv->active_content_table, "active-content-table");

  bin = nbtk_bin_new ();
  label = nbtk_label_new (_("People talking to you ..."));
  clutter_actor_set_name ((ClutterActor *)label, "active-content-header-label");
  nbtk_bin_set_child (NBTK_BIN (bin), (ClutterActor *)label);
  nbtk_bin_set_alignment (NBTK_BIN (bin), NBTK_ALIGN_START, NBTK_ALIGN_MIDDLE);
  nbtk_bin_set_fill (NBTK_BIN (bin), FALSE, TRUE);
  clutter_actor_set_name ((ClutterActor *)bin, "active-content-header");

  nbtk_table_add_actor (NBTK_TABLE (priv->active_content_table),
                        (ClutterActor *)bin,
                        0,
                        0);

  nbtk_table_add_actor (NBTK_TABLE (priv->active_content_table),
                        (ClutterActor *)priv->active_tile_view,
                        1,
                        0);

  nbtk_table_add_actor (NBTK_TABLE (priv->content_table),
                        (ClutterActor *)priv->active_content_table,
                        0,
                        0);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->content_table),
                               (ClutterActor *)priv->active_content_table,
                               "allocate-hidden", FALSE,
                               NULL);

  /* main area */
  scroll_view = nbtk_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll_view),
                               (ClutterActor *)priv->tile_view);

  scroll_bin = nbtk_table_new ();
  nbtk_table_add_actor (NBTK_TABLE (scroll_bin),
                        (ClutterActor *)scroll_view,
                        0,
                        0);
  nbtk_table_add_actor (NBTK_TABLE (priv->content_table),
                        (ClutterActor *)scroll_bin,
                        1,
                        0);
  clutter_actor_set_name ((ClutterActor *)scroll_bin, "people-scroll-bin");


  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        (ClutterActor *)priv->content_table,
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

  /* No people && no accounts enabled */
  priv->no_people_tile =
    _make_empty_people_tile (self,
                             clutter_actor_get_width ((ClutterActor *)scroll_view));

  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        (ClutterActor *)priv->no_people_tile,
                                        1,
                                        0,
                                        "x-fill", TRUE,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "y-fill", FALSE,
                                        "y-align", 0.0,
                                        "row-span", 1,
                                        "col-span", 2,
                                        NULL);


  /* No people && acounts are online */
  priv->everybody_offline_tile =
    _make_everybody_offline_tile (self,
                                  clutter_actor_get_width ((ClutterActor *)scroll_view));
  clutter_actor_hide ((ClutterActor *)priv->everybody_offline_tile);

  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        (ClutterActor *)priv->everybody_offline_tile,
                                        1,
                                        0,
                                        "x-fill", TRUE,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "y-fill", FALSE,
                                        "y-align", 0.0,
                                        "row-span", 1,
                                        "col-span", 2,
                                        NULL);

  priv->selection_pane = nbtk_table_new ();
  clutter_actor_set_height ((ClutterActor *)priv->selection_pane, 220);
  clutter_actor_set_name ((ClutterActor *)priv->selection_pane, "people-selection-pane");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        (ClutterActor *)priv->selection_pane,
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

  label = nbtk_label_new (_("Selection"));
  clutter_actor_set_name ((ClutterActor *)label, "people-selection-label");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->selection_pane),
                                        (ClutterActor *)label,
                                        0,
                                        0,
                                        "x-fill",
                                        FALSE,
                                        "y-fill",
                                        FALSE,
                                        "x-expand",
                                        FALSE,
                                        "y-expand",
                                        FALSE,
                                        "x-align",
                                        0.0,
                                        "y-align",
                                        0.0,
                                        NULL);

  priv->control_box = nbtk_table_new ();
  nbtk_table_set_row_spacing (NBTK_TABLE (priv->control_box), 6);
  clutter_actor_set_width ((ClutterActor *)priv->control_box, 200);
  clutter_actor_set_name ((ClutterActor *)priv->control_box, "people-control-box");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->selection_pane),
                                        (ClutterActor *)priv->control_box,
                                        1,
                                        0,
                                        "x-fill",
                                        TRUE,
                                        "y-fill",
                                        TRUE,
                                        "x-expand",
                                        TRUE,
                                        "y-expand",
                                        TRUE,
                                        "x-align",
                                        0.0,
                                        "y-align",
                                        0.0,
                                        NULL);

  priv->nobody_selected_box = nbtk_bin_new ();
  clutter_actor_set_width ((ClutterActor *)priv->nobody_selected_box, 200);
  clutter_actor_set_name ((ClutterActor *)priv->nobody_selected_box,
                          "people-nobody-selected");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->selection_pane),
                                        (ClutterActor *)priv->nobody_selected_box,
                                        1,
                                        0,
                                        "x-fill",
                                        TRUE,
                                        "y-fill",
                                        TRUE,
                                        "x-expand",
                                        TRUE,
                                        "y-expand",
                                        TRUE,
                                        "x-align",
                                        0.0,
                                        "y-align",
                                        0.0,
                                        NULL);
  label = nbtk_label_new (_("Nobody selected"));
  clutter_actor_set_name ((ClutterActor *)label,
                          "people-nobody-selected-label");
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->nobody_selected_box),
                               (ClutterActor *)label);
  nbtk_bin_set_alignment (NBTK_BIN (priv->nobody_selected_box),
                          NBTK_ALIGN_MIDDLE,
                          NBTK_ALIGN_START);
  nbtk_bin_set_fill (NBTK_BIN (priv->nobody_selected_box),
                     FALSE,
                     FALSE);

  priv->selected_tile = anerley_tile_new (NULL);
  clutter_actor_set_height ((ClutterActor *)priv->selected_tile, 90);
  clutter_actor_set_name (CLUTTER_ACTOR (priv->selected_tile),
                          "people-selected-item");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->control_box),
                                        (ClutterActor *)priv->selected_tile,
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

  priv->primary_button = nbtk_button_new_with_label ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->primary_button), "people-primary-action");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->control_box),
                                        (ClutterActor *)priv->primary_button,
                                        1,
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
                                        "allocate-hidden",
                                        FALSE,
                                        NULL);

  priv->secondary_button = nbtk_button_new_with_label ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->secondary_button), "people-secondary-action");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (priv->control_box),
                                        (ClutterActor *)priv->secondary_button,
                                        2,
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

  _update_selected_item (self, NULL);

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
  g_signal_connect_after (priv->active_tile_view,
                          "selection-changed",
                          (GCallback)_active_tile_view_selection_changed_cb,
                          self);

  g_signal_connect (priv->tile_view,
                    "item-activated",
                    (GCallback)_tile_view_item_activated_cb,
                    self);

  g_signal_connect (priv->active_tile_view,
                    "item-activated",
                    (GCallback)_tile_view_item_activated_cb,
                    self);

  /* Put into the no people state */
  clutter_actor_hide ((ClutterActor *)priv->selection_pane);
  clutter_actor_hide ((ClutterActor *)priv->content_table);

  g_signal_connect (priv->model,
                    "bulk-change-end",
                    (GCallback)_model_bulk_changed_end_cb,
                    self);
  g_signal_connect (priv->active_model,
                    "bulk-change-end",
                    (GCallback)_active_model_bulk_change_end_cb,
                    self);

  /* Placeholder changes based on onlineness or not */
  _update_placeholder_state (self);

  g_signal_connect (priv->tp_feed,
                    "notify::accounts-online",
                    (GCallback)_tp_feed_online_notify_cb,
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


