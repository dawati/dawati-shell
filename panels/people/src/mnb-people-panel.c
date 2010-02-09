/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 * Derived from status panel, author: Emmanuele Bassi <ebassi@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
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
#include <anerley/anerley-presence-chooser.h>

#include <libebook/e-book.h>

#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-entry.h>

#include "mnb-people-panel.h"

G_DEFINE_TYPE (MnbPeoplePanel, mnb_people_panel, MX_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_PEOPLE_PANEL, MnbPeoplePanelPrivate))

#define TIMEOUT 250


typedef struct _MnbPeoplePanelPrivate MnbPeoplePanelPrivate;

struct _MnbPeoplePanelPrivate {
  AnerleyFeed *tp_feed;
  guint filter_timeout_id;
  AnerleyFeedModel *model;
  AnerleyFeedModel *active_model;
  ClutterActor *tex;
  ClutterActor *entry;
  GAppInfo *app_info;
  GtkIconTheme *icon_theme;
  MplPanelClient *panel_client;
  ClutterActor *tile_view;
  ClutterActor *active_tile_view;
  ClutterActor *content_table;
  ClutterActor *active_content_table;
  ClutterActor *everybody_offline_tile;
  ClutterActor *offline_banner;
  ClutterActor *header_box;
  ClutterActor *no_people_tile;
  ClutterActor *presence_chooser;
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
dropdown_show_cb (MplPanelClient *client,
                  gpointer        userdata)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);

  /* give focus to the actor */
  clutter_actor_grab_key_focus (priv->entry);
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
  mx_stylable_set_style_pseudo_class (MX_STYLABLE (actor),
                                      "hover");

  return FALSE;
}

static gboolean
_leave_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  mx_stylable_set_style_pseudo_class (MX_STYLABLE (actor),
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
      mpl_panel_client_hide (priv->panel_client);
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
_make_empty_people_tile (MnbPeoplePanel *people_panel)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (people_panel);

  ClutterActor *tile;
  ClutterActor *frame;
  ClutterActor *label;
  ClutterActor *hbox;
  ClutterActor *tmp_text;

  tile = mx_table_new ();
  mx_table_set_row_spacing (MX_TABLE (tile), 8);

  clutter_actor_set_name ((ClutterActor *)tile,
                          "people-people-pane-no-people-tile");
  frame = mx_frame_new ();
  clutter_actor_set_name (frame,
                          "people-no-people-message-bin");
  label = mx_label_new (_("Sorry, we can't find any people. " \
                          "Have you set up a Messenger account?"));
  clutter_actor_set_name (label,
                          "people-people-pane-main-label");
  tmp_text = mx_label_get_clutter_text (MX_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);
  mx_bin_set_child (MX_BIN (frame), label);
  mx_table_add_actor_with_properties (MX_TABLE (tile),
                                      frame,
                                      0,
                                      0,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", FALSE,
                                      "x-align", 0.0,
                                      NULL);
  mx_bin_set_alignment (MX_BIN (frame), MX_ALIGN_START, MX_ALIGN_MIDDLE);

  priv->app_info = (GAppInfo *)g_desktop_app_info_new ("empathy-accounts.desktop");

  if (priv->app_info)
  {
    hbox = mx_table_new ();
    clutter_actor_set_name (hbox,
                            "people-no-people-launcher");
    mx_table_set_col_spacing (MX_TABLE (hbox), 8);
    mx_table_add_actor_with_properties (MX_TABLE (tile),
                                        (ClutterActor *)hbox,
                                        1,
                                        0,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        NULL);


    priv->tex = clutter_texture_new ();
    clutter_actor_set_size (priv->tex, ICON_SIZE, ICON_SIZE);
    mx_table_add_actor_with_properties (MX_TABLE (hbox),
                                        priv->tex,
                                        1,
                                        0,
                                        "x-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "y-expand", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);

    label = mx_label_new (g_app_info_get_description (priv->app_info));
    clutter_actor_set_name (label, "people-no-people-description");
    mx_table_add_actor_with_properties (MX_TABLE (hbox),
                                          (ClutterActor *)label,
                                          1,
                                          1,
                                          "x-expand", TRUE,
                                          "x-fill", FALSE,
                                          "y-expand", FALSE,
                                          "y-fill", FALSE,
                                          "x-align", 0.0,
                                          "y-align", 0.5,
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

static ClutterActor *
_make_everybody_offline_tile (MnbPeoplePanel *pane)
{
  ClutterActor *tile;
  ClutterActor *label, *bin;
  ClutterActor *tmp_text;

  tile = mx_table_new ();
  mx_table_set_row_spacing (MX_TABLE (tile), 8);

  clutter_actor_set_name ((ClutterActor *)tile,
                          "people-pane-everybody-offline-tile");
  label = mx_label_new (_("Sorry, we can't find any people. " \
                          "It looks like they are all offline."));
  clutter_actor_set_name ((ClutterActor *)label,
                          "people-pane-everybody-offline-label");
  tmp_text = mx_label_get_clutter_text (MX_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);

  bin = mx_frame_new ();
  mx_bin_set_child (MX_BIN (bin), label);
  mx_bin_set_alignment (MX_BIN (bin), MX_ALIGN_START, MX_ALIGN_MIDDLE);
  mx_bin_set_fill (MX_BIN (bin), FALSE, TRUE);
  clutter_actor_set_name (bin,
                          "people-pane-everybody-offline-bin");

  mx_table_add_actor_with_properties (MX_TABLE (tile),
                                      (ClutterActor *)bin,
                                      0,
                                      0,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", FALSE,
                                      "x-align", 0.0,
                                      NULL);
  return tile;
}

static ClutterActor *
_make_offline_banner (MnbPeoplePanel *pane,
                      gint            width)
{
  ClutterActor *tile;
  ClutterActor *tmp_text;
  ClutterActor *label, *bin;

  tile = mx_table_new ();
  mx_table_set_row_spacing (MX_TABLE (tile), 8);

  clutter_actor_set_width (tile, width);
  clutter_actor_set_name (tile,
                          "people-pane-you-offline-banner");
  label = mx_label_new (_("To see your IM contacts, "
                          "go online in the Status panel."));
  clutter_actor_set_name (label,
                          "people-pane-you-offline-label");
  tmp_text = mx_label_get_clutter_text (MX_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);

  bin = mx_frame_new ();
  mx_bin_set_child (MX_BIN (bin), (label));
  mx_bin_set_alignment (MX_BIN (bin), MX_ALIGN_START, MX_ALIGN_MIDDLE);
  mx_bin_set_fill (MX_BIN (bin), FALSE, TRUE);
  clutter_actor_set_name (bin,
                          "people-pane-you-offline-bin");

  mx_table_add_actor_with_properties (MX_TABLE (tile),
                                      (ClutterActor *)bin,
                                      0,
                                      0,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", FALSE,
                                      "x-align", 0.0,
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
    mx_table_set_row_spacing (MX_TABLE (priv->content_table), 6);

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
    mx_table_set_row_spacing (MX_TABLE (priv->content_table), 0);
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
_edit_contact_action (MnbPeoplePanel *panel,
                      AnerleyItem    *item)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (panel);
  gchar *command_line;
  const gchar *uid;

  if (priv->panel_client)
  {
    uid = anerley_econtact_item_get_uid ((AnerleyEContactItem *)item);
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
        mpl_panel_client_hide (priv->panel_client);
    }
  }
}

static void
_tile_view_item_activated_cb (AnerleyTileView *view,
                              AnerleyItem     *item,
                              gpointer         userdata)
{
  MnbPeoplePanel *panel = MNB_PEOPLE_PANEL (userdata);
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);

  if (ANERLEY_IS_ECONTACT_ITEM (item) &&
      anerley_econtact_item_get_email (ANERLEY_ECONTACT_ITEM (item)) == NULL)
  {
    _edit_contact_action (panel, item);
  } else {
    anerley_item_activate (item);
  }

  if (priv->panel_client)
    mpl_panel_client_hide (priv->panel_client);
}

static void
_update_placeholder_state (MnbPeoplePanel *self)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);
  gint accounts_available, accounts_online;

  g_object_get (priv->tp_feed,
                "accounts-available", &accounts_available,
                "accounts-online", &accounts_online,
                NULL);

  /* There is something in the model, hide all placeholders */
  if (clutter_model_get_first_iter (CLUTTER_MODEL (priv->model)))
  {
    clutter_actor_hide (priv->no_people_tile);
    clutter_actor_hide (priv->everybody_offline_tile);

    /* Ensure content stuff is visible */
    clutter_actor_show (priv->content_table);

    if (accounts_available > 0 && accounts_online == 0)
    {
      clutter_actor_show (priv->offline_banner);
      mx_table_set_row_spacing (MX_TABLE (self), 6);
    } else {
      clutter_actor_hide (priv->offline_banner);
      /* Because allocate-hidden=false still causes row spacing to be applied,
       * so halve it.
       */
      mx_table_set_row_spacing (MX_TABLE (self), 3);
    }
  } else {
    /* Hide real content stuff */
    clutter_actor_hide (priv->content_table);

    mx_table_set_row_spacing (MX_TABLE (self), 6);

    if (accounts_online == 0)
    {
      if (accounts_available == 0)
      {
        clutter_actor_show (priv->no_people_tile);
        clutter_actor_hide (priv->everybody_offline_tile);
        clutter_actor_hide (priv->offline_banner);
      } else {
        clutter_actor_show (priv->offline_banner);
        clutter_actor_hide (priv->no_people_tile);
        clutter_actor_hide (priv->everybody_offline_tile);
      }
    } else {
      clutter_actor_show (priv->everybody_offline_tile);
      clutter_actor_hide (priv->no_people_tile);
      clutter_actor_hide (priv->offline_banner);
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
_tp_feed_available_notify_cb (GObject    *object,
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
  ClutterActor *label;
  ClutterActor *scroll_view, *scroll_bin, *bin;
  AnerleyFeed *feed, *ebook_feed, *active_feed;
  EBook *book;
  GError *error = NULL;

  mx_table_set_col_spacing (MX_TABLE (self), 4);
  mx_table_set_row_spacing (MX_TABLE (self), 6);
  clutter_actor_set_name (CLUTTER_ACTOR (self), "people-vbox");

  priv->header_box = mx_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (priv->header_box), "people-search");
  mx_table_set_col_spacing (MX_TABLE (priv->header_box), 20);
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      CLUTTER_ACTOR (priv->header_box),
                                      0, 0,
                                      "row-span", 1,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", TRUE,
                                      "x-align", 0.0,
                                      "y-align", 0.0,
                                      NULL);

  label = mx_label_new (_("People"));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "people-search-label");
  mx_table_add_actor_with_properties (MX_TABLE (priv->header_box),
                                      CLUTTER_ACTOR (label),
                                      0, 0,
                                      "x-expand", FALSE,
                                      "y-expand", FALSE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      "x-align", 0.0,
                                      "y-align", 0.5,
                                      NULL);

  priv->entry = (ClutterActor *) mpl_entry_new (_("Search"));
  clutter_actor_set_name (CLUTTER_ACTOR (priv->entry), "people-search-entry");
  clutter_actor_set_width (CLUTTER_ACTOR (priv->entry), 600);
  mx_table_add_actor_with_properties (MX_TABLE (priv->header_box),
                                      CLUTTER_ACTOR (priv->entry),
                                      0, 1,
                                      "x-expand", FALSE,
                                      "y-expand", FALSE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      "x-align", 0.0,
                                      "y-align", 0.5,
                                      NULL);
  priv->presence_chooser = anerley_presence_chooser_new ();
  mx_table_add_actor_with_properties (MX_TABLE (priv->header_box),
                                      priv->presence_chooser,
                                      0, 2,
                                      "x-expand", FALSE,
                                      "x-fill", FALSE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
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

  priv->content_table = mx_table_new ();

  /* active conversations */
  priv->active_content_table = mx_table_new ();

  clutter_actor_hide (priv->active_content_table);
  clutter_actor_set_name (priv->active_content_table, "active-content-table");

  bin = mx_frame_new ();
  label = mx_label_new (_("People talking to you ..."));
  clutter_actor_set_name (label, "active-content-header-label");
  mx_bin_set_child (MX_BIN (bin), label);
  mx_bin_set_alignment (MX_BIN (bin), MX_ALIGN_START, MX_ALIGN_MIDDLE);
  mx_bin_set_fill (MX_BIN (bin), FALSE, TRUE);
  clutter_actor_set_name (bin, "active-content-header");

  mx_table_add_actor (MX_TABLE (priv->active_content_table),
                      bin,
                      0,
                      0);

  mx_table_add_actor (MX_TABLE (priv->active_content_table),
                      priv->active_tile_view,
                      1,
                      0);

  mx_table_add_actor (MX_TABLE (priv->content_table),
                      priv->active_content_table,
                      0,
                      0);

  /* main area */
  scroll_view = mx_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll_view),
                               priv->tile_view);

  scroll_bin = mx_table_new ();
  mx_table_add_actor (MX_TABLE (scroll_bin),
                      scroll_view,
                      0,
                      0);
  mx_table_add_actor (MX_TABLE (priv->content_table),
                      scroll_bin,
                      1,
                      0);
  clutter_actor_set_name (scroll_bin, "people-scroll-bin");


  /* No people && no accounts enabled */
  priv->no_people_tile = _make_empty_people_tile (self);

  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->no_people_tile,
                                      1, 0,
                                      "x-fill", TRUE,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      "y-align", 0.0,
                                      "row-span", 1,
                                      NULL);

  /* No people && acounts are online */
  priv->everybody_offline_tile = _make_everybody_offline_tile (self);
  clutter_actor_hide (priv->everybody_offline_tile);

  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->everybody_offline_tile,
                                      1, 0,
                                      "x-fill", TRUE,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      "y-align", 0.0,
                                      "row-span", 1,
                                      NULL);

  priv->offline_banner =
    _make_offline_banner (self,
                          clutter_actor_get_width (scroll_view));
  clutter_actor_hide (priv->offline_banner);
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->offline_banner,
                                      1, 0,
                                      "x-fill", TRUE,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      "y-align", 0.0,
                                      "row-span", 1,
                                      NULL);

  /* Real content stuff */
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      priv->content_table,
                                      2, 0,
                                      "x-fill", TRUE,
                                      "x-expand", TRUE,
                                      "y-expand", TRUE,
                                      "y-fill", TRUE,
                                      "row-span", 1,
                                      NULL);

  g_signal_connect (priv->tile_view,
                    "item-activated",
                    (GCallback)_tile_view_item_activated_cb,
                    self);

  g_signal_connect (priv->active_tile_view,
                    "item-activated",
                    (GCallback)_tile_view_item_activated_cb,
                    self);

  /* Put into the no people state */
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
  g_signal_connect (priv->tp_feed,
                    "notify::accounts-available",
                    (GCallback)_tp_feed_available_notify_cb,
                    self);
}

ClutterActor *
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


