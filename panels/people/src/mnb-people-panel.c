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
#include <anerley/anerley-compact-tile-view.h>
#include <anerley/anerley-tile.h>
#include <anerley/anerley-compact-tile.h>
#include <anerley/anerley-tp-monitor-feed.h>
#include <anerley/anerley-tp-item.h>
#include <anerley/anerley-presence-chooser.h>
#include <anerley/anerley-tp-user-avatar.h>

#include <dawati-panel/mpl-panel-clutter.h>
#include <dawati-panel/mpl-panel-common.h>
#include <dawati-panel/mpl-entry.h>

#include "mnb-people-panel.h"
#include "sw-online.h"

G_DEFINE_TYPE (MnbPeoplePanel, mnb_people_panel, MX_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_PEOPLE_PANEL, MnbPeoplePanelPrivate))

#define TIMEOUT 250


#define PLACEHOLDER_MESSAGE _("When you have an IM service configured " \
                              "(like Messenger), this is where you chat to " \
                              "people.")
#define PLACEHOLDER_IMAGE THEMEDIR "/people.png"

typedef struct _MnbPeoplePanelPrivate MnbPeoplePanelPrivate;

struct _MnbPeoplePanelPrivate {
  FolksIndividualAggregator *aggregator;
  TpAccountManager *am;
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
  ClutterActor *active_list_view;
  ClutterActor *content_table;
  ClutterActor *side_table;
  ClutterActor *active_content_table;
  ClutterActor *everybody_offline_tile;
  ClutterActor *offline_banner;
  ClutterActor *header_box;
  ClutterActor *no_people_tile;
  ClutterActor *presence_chooser;
  ClutterActor *me_table;
  ClutterActor *main_scroll_view;
  ClutterActor *avatar;
  ClutterActor *sort_by_chooser;
  ClutterActor *new_chooser;
  ClutterActor *search_entry;
  ClutterActor *banner_box;
};

static void _online_notify_cb (gboolean online,
                               gpointer userdata);

static void
mnb_people_panel_dispose (GObject *object)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (object);

  g_clear_object (&priv->panel_client);
  g_clear_object (&priv->aggregator);
  g_clear_object (&priv->am);

  G_OBJECT_CLASS (mnb_people_panel_parent_class)->dispose (object);
}

static void
mnb_people_panel_finalize (GObject *object)
{
  sw_online_remove_notify (_online_notify_cb, object);

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

#define ICON_SIZE 48

static void
_settings_launcher_button_clicked_cb (MxButton *button,
                                      gpointer  userdata)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);
  GDesktopAppInfo *app_info;
  GError *error = NULL;
  const gchar *args[3] = { NULL, };

  app_info = g_desktop_app_info_new ("gnome-control-center.desktop");
  args[0] = g_app_info_get_commandline (G_APP_INFO (app_info));
  args[1] = "empathy-accounts.desktop";
  args[2] = NULL;

  if (!g_spawn_async (NULL,
                      (gchar **)args,
                      NULL,
                      G_SPAWN_SEARCH_PATH,
                      NULL,
                      NULL,
                      NULL,
                      &error))
  {
    g_warning (G_STRLOC ": Error starting control center for empathy-accounts: %s",
               error->message);
    g_clear_error (&error);
  } else {
    if (priv->panel_client)
      mpl_panel_client_hide (priv->panel_client);
  }

  g_object_unref (app_info);
}

ClutterActor *
_make_settings_launcher (MnbPeoplePanel *people_panel)
{
  ClutterActor *table;
  ClutterActor *icon_tex;
  ClutterActor *button;
  GAppInfo *app_info;
  gchar *button_str;

  app_info = (GAppInfo *)g_desktop_app_info_new ("empathy-accounts.desktop");

  
  table = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (table), 16);
  app_info = (GAppInfo *)g_desktop_app_info_new ("empathy-accounts.desktop");

  icon_tex = g_object_new (MX_TYPE_ICON,
                           "icon-name", "netbook-empathy-accounts",
                           NULL);

  mx_table_add_actor_with_properties (MX_TABLE (table),
                                      icon_tex,
                                      0, 0,
                                      "x-expand", FALSE,
                                      "y-expand", TRUE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      NULL);



  button_str = g_strdup_printf (_("Open %s"),
                                g_app_info_get_name (app_info));

  button = mx_button_new_with_label (button_str);
  g_free (button_str);
  g_signal_connect (button,
                    "clicked",
                    (GCallback)_settings_launcher_button_clicked_cb,
                    people_panel);

  mx_table_add_actor_with_properties (MX_TABLE (table),
                                      button,
                                      0, 1,
                                      "x-expand", FALSE,
                                      "y-expand", TRUE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      NULL);
  g_object_unref (app_info);

  return table;
}

static ClutterActor *
_make_empty_people_tile (MnbPeoplePanel *people_panel)
{
  ClutterActor *tile;
  ClutterActor *frame;
  ClutterActor *label;
  ClutterActor *tmp_text;
  ClutterActor *settings_launcher;
  ClutterActor *picture;

  tile = mx_table_new ();
  mx_table_set_row_spacing (MX_TABLE (tile), 8);

  clutter_actor_set_name ((ClutterActor *)tile,
                          "people-pane-no-people-tile");
  /* title */
  frame = mx_frame_new ();
  clutter_actor_set_name (frame,
                          "people-no-people-message-bin");
  label = mx_label_new_with_text (_("This is the People panel."));
  clutter_actor_set_name (label,
                          "people-no-people-message-title");
  mx_bin_set_child (MX_BIN (frame), label);
  mx_table_add_actor_with_properties (MX_TABLE (tile),
                                      frame,
                                      0, 0,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      NULL);
  mx_bin_set_alignment (MX_BIN (frame), MX_ALIGN_START, MX_ALIGN_MIDDLE);

  /* message */
  frame = mx_frame_new ();
  clutter_actor_set_name (frame,
                          "people-no-people-message-bin");
  label = mx_label_new_with_text (PLACEHOLDER_MESSAGE);
  clutter_actor_set_name (label,
                          "people-no-people-message-label");
  tmp_text = mx_label_get_clutter_text (MX_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);
  clutter_actor_set_width (label, 500);

  mx_bin_set_child (MX_BIN (frame), label);

  mx_table_add_actor_with_properties (MX_TABLE (tile),
                                      frame,
                                      1, 0,
                                      "x-expand", TRUE,
                                      "y-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-fill", TRUE,
                                      "x-align", MX_ALIGN_START,
                                      NULL);
  mx_bin_set_alignment (MX_BIN (frame), MX_ALIGN_START, MX_ALIGN_MIDDLE);

  settings_launcher = _make_settings_launcher (people_panel);

  clutter_actor_set_name (settings_launcher,
                          "people-panel-settings-launcher-tile");

  mx_table_add_actor_with_properties (MX_TABLE (tile),
                                      settings_launcher,
                                      2, 0,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      NULL);

  picture = clutter_texture_new_from_file (PLACEHOLDER_IMAGE, NULL);

  mx_table_add_actor_with_properties (MX_TABLE (tile),
                                      picture,
                                      3, 0,
                                      "x-expand", TRUE,
                                      "y-expand", TRUE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      "x-align", MX_ALIGN_END,
                                      "y-align", MX_ALIGN_END,
                                      NULL);



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
  label = mx_label_new_with_text (_("Sorry, we can't find any people. " \
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
                                      "x-align", MX_ALIGN_START,
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
  label = mx_label_new_with_text (_("To see your IM contacts, "
                                    "you need to go online."));
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
                                      "x-align", MX_ALIGN_START,
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
//    mx_table_set_row_spacing (MX_TABLE (priv->content_table), 6);

    if (priv->panel_client)
    {
      mpl_panel_client_request_button_style (priv->panel_client,
                                             "people-button-active");
      number_active_people = clutter_model_get_n_rows (CLUTTER_MODEL (priv->active_model));

      if (number_active_people > 1)
      {
        tooltip = g_strdup_printf (_("people - you are chatting with %d people"),
                                   number_active_people);
        mpl_panel_client_request_tooltip (priv->panel_client,
                                          tooltip);
        g_free (tooltip);
      } else {
        mpl_panel_client_request_tooltip (priv->panel_client,
                                          _("people - you are chatting with someone"));
      }
    }
  } else {
 //   mx_table_set_row_spacing (MX_TABLE (priv->content_table), 0);

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
_tile_view_item_activated_cb (AnerleyTileView *view,
                              AnerleyItem     *item,
                              gpointer         userdata)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);

  anerley_item_activate (item);

  if (priv->panel_client)
    mpl_panel_client_hide (priv->panel_client);
}

static void
_update_placeholder_state (MnbPeoplePanel *self)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);
  gint accounts_available = 0;
  gint accounts_online = 0;
  GList *accounts;

  accounts = tp_account_manager_get_valid_accounts (priv->am);
  while (accounts != NULL)
  {
    TpAccount *account = accounts->data;

    accounts_available++;

    if (tp_account_get_connection_status (account, NULL) ==
        TP_CONNECTION_STATUS_CONNECTED)
      accounts_online++;

    accounts = g_list_delete_link (accounts, accounts);
  }

  clutter_actor_set_name (priv->content_table, "people-panel-content-box");

  /* There is something in the model, hide all placeholders */
  if (clutter_model_get_first_iter (CLUTTER_MODEL (priv->model)))
  {
    clutter_actor_hide (priv->no_people_tile);
    clutter_actor_hide (priv->everybody_offline_tile);

    /* Ensure content stuff is visible */
    clutter_actor_show (priv->main_scroll_view);
    clutter_actor_show (priv->me_table);

    if (accounts_available > 0 && accounts_online == 0)
    {
      clutter_actor_show (priv->offline_banner);
    } else {
      clutter_actor_hide (priv->offline_banner);
    }
  } else {
    /* Hide real content stuff */
    clutter_actor_hide (priv->main_scroll_view);

    if (accounts_online == 0)
    {
      if (accounts_available == 0)
      {
        clutter_actor_set_name (priv->content_table,
                                "no-people-panel-content-box");
        clutter_actor_show (priv->no_people_tile);
        clutter_actor_hide (priv->me_table);
        clutter_actor_hide (priv->everybody_offline_tile);
        clutter_actor_hide (priv->offline_banner);
      } else {
        clutter_actor_show (priv->me_table);
        clutter_actor_show (priv->offline_banner);
        clutter_actor_hide (priv->no_people_tile);
        clutter_actor_hide (priv->everybody_offline_tile);
      }
    } else {
      clutter_actor_show (priv->me_table);
      clutter_actor_show (priv->everybody_offline_tile);
      clutter_actor_hide (priv->no_people_tile);
      clutter_actor_hide (priv->offline_banner);
    }
  }
}

static void
_update_presence_chooser_state (MnbPeoplePanel *panel)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (panel);
  GList *accounts;

  accounts = tp_account_manager_get_valid_accounts (priv->am);

  if (accounts == NULL)
  {
    clutter_actor_hide (priv->presence_chooser);
  } else {
    if (sw_is_online ())
    {
      clutter_actor_show (priv->presence_chooser);
    } else {
      clutter_actor_hide (priv->presence_chooser);
    }
  }

  g_list_free (accounts);
}

static void
_account_status_changed_cb (GObject    *object,
                           GParamSpec *pspec,
                           gpointer    userdata)
{
  _update_placeholder_state (MNB_PEOPLE_PANEL (userdata));
  _update_presence_chooser_state (MNB_PEOPLE_PANEL (userdata));
}

static void
_account_validity_changed_cb (TpAccountManager *manager,
                              TpAccount        *account,
                              gboolean          valid,
                              gpointer          user_data)
{
  MnbPeoplePanel *self = user_data;

  if (valid)
  {
    tp_g_signal_connect_object (account, "status-changed",
                                G_CALLBACK (_account_status_changed_cb),
                                self, 0);
  }

  _update_placeholder_state (self);
  _update_presence_chooser_state (self);
}

static void
_model_bulk_changed_end_cb (AnerleyFeedModel *model,
                            gpointer          userdata)
{
  _update_placeholder_state (MNB_PEOPLE_PANEL (userdata));
}

static void
_update_online_state (MnbPeoplePanel *panel,
                      gboolean        online)
{
  _update_presence_chooser_state (panel);
}

static void
am_prepared_cb (GObject *object,
                GAsyncResult *result,
                gpointer user_data)
{
  MnbPeoplePanel *self= user_data;
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);
  GList *accounts;
  GError *error = NULL;

  if (!tp_proxy_prepare_finish (object, result, &error))
  {
    g_warning ("Error preparing the TpAccountManager: %s", error->message);
    g_clear_error (&error);
    return;
  }

  g_signal_connect (priv->am,
                    "account-validity-changed",
                    (GCallback)_account_validity_changed_cb,
                    self);

  accounts = tp_account_manager_get_valid_accounts (priv->am);
  while (accounts != NULL)
  {
    tp_g_signal_connect_object (accounts->data, "status-changed",
                                G_CALLBACK (_account_status_changed_cb),
                                self, 0);
    accounts = g_list_delete_link (accounts, accounts);
  }

  _update_placeholder_state (self);
  _update_presence_chooser_state (self);
}

static void
sort_by_index_changed_cb (MxComboBox     *combo,
                          GParamSpec     *pspec,
                          MnbPeoplePanel *self)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);
  gint index = mx_combo_box_get_index (combo);

  if (index == 4)
    anerley_feed_model_set_show_offline (priv->model, FALSE);
  else if (index == 5)
    anerley_feed_model_set_show_offline (priv->model, TRUE);
}

static void
create_sort_by_chooser (MnbPeoplePanel *self)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);

  priv->sort_by_chooser = mx_combo_box_new ();
  mx_combo_box_append_text (MX_COMBO_BOX (priv->sort_by_chooser), _("Sort by:"));
  mx_combo_box_append_text (MX_COMBO_BOX (priv->sort_by_chooser), _("- Top contacts"));
  mx_combo_box_append_text (MX_COMBO_BOX (priv->sort_by_chooser), _("- Name"));
  mx_combo_box_append_text (MX_COMBO_BOX (priv->sort_by_chooser), _("Show:"));
  mx_combo_box_append_text (MX_COMBO_BOX (priv->sort_by_chooser), _("- Only online"));
  mx_combo_box_append_text (MX_COMBO_BOX (priv->sort_by_chooser), _("- All contacts"));

  mx_combo_box_set_active_text (MX_COMBO_BOX (priv->sort_by_chooser), _("Sort by"));

  g_signal_connect (priv->sort_by_chooser,
                    "notify::index",
                    G_CALLBACK (sort_by_index_changed_cb),
                    self);
}

static void
create_new_chooser (MnbPeoplePanel *self)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);

  priv->new_chooser = mx_combo_box_new ();
  mx_combo_box_append_text (MX_COMBO_BOX (priv->new_chooser), _("New contact"));
  mx_combo_box_append_text (MX_COMBO_BOX (priv->new_chooser), _("New group chat"));

  mx_combo_box_set_active_text (MX_COMBO_BOX (priv->new_chooser), _("New"));
}

static void
search_entry_text_changed_cb (MxEntry        *entry,
                              GParamSpec     *pspec,
                              MnbPeoplePanel *self)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);
  const gchar *text = mx_entry_get_text (entry);

  if (text != NULL && text[0] == '\0')
    text = NULL;

  anerley_feed_model_set_filter_text (priv->model, text);
}

static void
create_search_entry (MnbPeoplePanel *self)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);

  priv->search_entry = mx_entry_new ();
  mx_entry_set_hint_text (MX_ENTRY (priv->search_entry), _("Search"));

  /* FIXME: wtf we can't set from icon name??
   * mx_entry_set_secondary_icon_from_file (priv->search_entry, "");
   */

  g_signal_connect (priv->search_entry,
                    "notify::text",
                    G_CALLBACK (search_entry_text_changed_cb),
                    self);
}

static void
mnb_people_panel_init (MnbPeoplePanel *self)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);
  ClutterActor *label;
  ClutterActor *scroll_view, *bin, *tmp_text;
  AnerleyFeed *active_feed;

  mx_table_set_column_spacing (MX_TABLE (self), 6);
  mx_table_set_row_spacing (MX_TABLE (self), 6);

  /* Populate top level table */
  priv->banner_box = mx_table_new ();
  clutter_actor_set_name (priv->banner_box, "people-panel-banner-box");
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      CLUTTER_ACTOR (priv->banner_box),
                                      0, 0,
                                      "column-span", 2,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", TRUE,
                                      "x-align", MX_ALIGN_START,
                                      "y-align", MX_ALIGN_START,
                                      NULL);

  priv->header_box = mx_table_new ();
  clutter_actor_set_name (priv->header_box, "people-panel-header-box");
  mx_table_set_column_spacing (MX_TABLE (priv->header_box), 20);
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      CLUTTER_ACTOR (priv->header_box),
                                      1, 0,
                                      "column-span", 2,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", TRUE,
                                      "x-align", MX_ALIGN_START,
                                      "y-align", MX_ALIGN_START,
                                      NULL);

  priv->side_table = mx_table_new ();
  clutter_actor_set_width (priv->side_table, 288);
  clutter_actor_set_name (priv->side_table, "people-panel-side-box");
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      CLUTTER_ACTOR (priv->side_table),
                                      2, 0,
                                      "x-expand", FALSE,
                                      "y-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-fill", TRUE,
                                      NULL);

  priv->content_table = mx_table_new ();
  clutter_actor_set_name (priv->content_table, "people-panel-content-box");
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      CLUTTER_ACTOR (priv->content_table),
                                      2, 1,
                                      "x-expand", TRUE,
                                      "y-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-fill", TRUE,
                                      NULL);

  /* Populate banner */
  label = mx_label_new_with_text (_("IM"));
  clutter_actor_set_name (label, "people-panel-banner-label");
  mx_table_add_actor_with_properties (MX_TABLE (priv->banner_box),
                                      CLUTTER_ACTOR (label),
                                      0, 0,
                                      "x-expand", TRUE,
                                      "y-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-fill", TRUE,
                                      NULL);

  /* Populate header */

  priv->me_table = mx_table_new ();

  mx_table_set_column_spacing (MX_TABLE (priv->me_table), 16);
  mx_table_set_row_spacing (MX_TABLE (priv->me_table), 8);

  clutter_actor_set_name (priv->me_table, "people-panel-me-box");

  priv->avatar = anerley_tp_user_avatar_new ();
  bin = mx_frame_new ();
  clutter_actor_set_name (bin, "people-panel-me-avatar-frame");
  mx_bin_set_child (MX_BIN (bin), priv->avatar);
  clutter_actor_set_size (priv->avatar, 48, 48);
  mx_bin_set_fill (MX_BIN (bin), TRUE, TRUE);
  mx_table_add_actor_with_properties (MX_TABLE (priv->me_table),
                                      bin,
                                      0, 0,
                                      "x-expand", FALSE,
                                      "y-expand", TRUE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      NULL);
  label = mx_label_new_with_text (_("Me"));
  clutter_actor_set_name (label, "people-panel-me-label");
  mx_table_add_actor_with_properties (MX_TABLE (priv->me_table),
                                      label,
                                      0, 1,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      NULL);
  mx_table_add_actor_with_properties (MX_TABLE (priv->header_box),
                                      CLUTTER_ACTOR (priv->me_table),
                                      0, 0,
                                      "x-expand", FALSE,
                                      "y-expand", FALSE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      NULL);
  priv->presence_chooser = anerley_presence_chooser_new ();
  mx_table_add_actor_with_properties (MX_TABLE (priv->header_box),
                                      priv->presence_chooser,
                                      0, 2,
                                      "x-expand", FALSE,
                                      "y-expand", TRUE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      NULL);
  create_sort_by_chooser (self);
  mx_table_add_actor_with_properties (MX_TABLE (priv->header_box),
                                      priv->sort_by_chooser,
                                      0, 3,
                                      "x-expand", FALSE,
                                      "y-expand", TRUE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      NULL);
  create_new_chooser (self);
  mx_table_add_actor_with_properties (MX_TABLE (priv->header_box),
                                      priv->new_chooser,
                                      0, 4,
                                      "x-expand", FALSE,
                                      "y-expand", TRUE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      NULL);
  create_search_entry (self);
  mx_table_add_actor_with_properties (MX_TABLE (priv->header_box),
                                      priv->search_entry,
                                      0, 5,
                                      "x-expand", TRUE,
                                      "y-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-fill", FALSE,
                                      NULL);
  /* Main body table */

  priv->am = tp_account_manager_dup ();
  priv->aggregator = folks_individual_aggregator_new ();
  priv->tp_feed = ANERLEY_FEED (anerley_tp_feed_new (priv->aggregator));

  priv->model = (AnerleyFeedModel *)anerley_feed_model_new (priv->tp_feed);
  priv->tile_view = anerley_tile_view_new (priv->model);

  priv->main_scroll_view = mx_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->main_scroll_view),
                               priv->tile_view);

  mx_table_add_actor (MX_TABLE (priv->content_table),
                      priv->main_scroll_view,
                      0,
                      0);

  /* No people && no accounts enabled */
  priv->no_people_tile = _make_empty_people_tile (self);
  clutter_actor_hide (priv->no_people_tile);

  mx_table_add_actor_with_properties (MX_TABLE (priv->content_table),
                                      priv->no_people_tile,
                                      1, 0,
                                      "x-fill", TRUE,
                                      "x-expand", TRUE,
                                      "y-expand", TRUE,
                                      "y-fill", TRUE,
                                      NULL);

  /* No people && acounts are online */
  priv->everybody_offline_tile = _make_everybody_offline_tile (self);
  clutter_actor_hide (priv->everybody_offline_tile);

  mx_table_add_actor_with_properties (MX_TABLE (priv->content_table),
                                      priv->everybody_offline_tile,
                                      2, 0,
                                      "x-fill", TRUE,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      "y-align", MX_ALIGN_START,
                                      "row-span", 1,
                                      NULL);

  priv->offline_banner =
    _make_offline_banner (self,
                          clutter_actor_get_width (priv->main_scroll_view));
  clutter_actor_hide (priv->offline_banner);
  mx_table_add_actor_with_properties (MX_TABLE (priv->content_table),
                                      priv->offline_banner,
                                      2, 0,
                                      "x-fill", TRUE,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "y-fill", FALSE,
                                      "y-align", MX_ALIGN_START,
                                      "row-span", 1,
                                      NULL);

  /* Side table */
  active_feed = anerley_tp_monitor_feed_new (priv->aggregator,
                                             "DawatiPanelPeople");
  priv->active_model = (AnerleyFeedModel *)anerley_feed_model_new (active_feed);


  priv->active_list_view = anerley_compact_tile_view_new (priv->active_model);
  scroll_view = mx_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll_view),
                               priv->active_list_view);

  /* active conversations */
  priv->active_content_table = mx_table_new ();
  clutter_actor_set_name (priv->active_content_table,
                          "people-panel-active-content-box");

  bin = mx_frame_new ();
  clutter_actor_set_name (bin, "people-panel-active-content-header");
  label = mx_label_new_with_text (_("You are chatting with:"));
  clutter_actor_set_name (label, "people-panel-active-content-header-label");
  tmp_text = mx_label_get_clutter_text (MX_LABEL (label));
  clutter_text_set_ellipsize (CLUTTER_TEXT(tmp_text), PANGO_ELLIPSIZE_NONE);
  mx_bin_set_child (MX_BIN (bin), label);
  mx_bin_set_alignment (MX_BIN (bin), MX_ALIGN_START, MX_ALIGN_MIDDLE);
  mx_bin_set_fill (MX_BIN (bin), TRUE, TRUE);

  mx_table_add_actor_with_properties (MX_TABLE (priv->active_content_table),
                                      bin,
                                      0, 0,
                                      "y-expand", FALSE,
                                      NULL);


  mx_table_add_actor (MX_TABLE (priv->active_content_table),
                      scroll_view,
                      1, 0);

  mx_table_add_actor_with_properties (MX_TABLE (priv->side_table),
                                      priv->active_content_table,
                                      0, 0,
                                      "x-expand", TRUE,
                                      NULL);

  g_signal_connect (priv->tile_view,
                    "item-activated",
                    (GCallback)_tile_view_item_activated_cb,
                    self);

  g_signal_connect (priv->active_list_view,
                    "item-activated",
                    (GCallback)_tile_view_item_activated_cb,
                    self);


  g_signal_connect (priv->model,
                    "bulk-change-end",
                    (GCallback)_model_bulk_changed_end_cb,
                    self);
  g_signal_connect (priv->active_model,
                    "bulk-change-end",
                    (GCallback)_active_model_bulk_change_end_cb,
                    self);
  clutter_actor_hide ((ClutterActor *)priv->main_scroll_view);

  /* Placeholder changes based on onlineness or not */
  _update_placeholder_state (self);

  sw_online_add_notify (_online_notify_cb, self);
  _update_online_state (self, sw_is_online ());

  tp_proxy_prepare_async (priv->am, NULL, am_prepared_cb, self);
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
}

static void
_online_notify_cb (gboolean online,
                   gpointer userdata)
{
  MnbPeoplePanel *panel = MNB_PEOPLE_PANEL (userdata);

  _update_online_state (panel, online);
}
