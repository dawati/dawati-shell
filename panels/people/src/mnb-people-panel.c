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
#include <anerley/anerley-aggregate-tp-feed.h>
#include <anerley/anerley-tp-monitor-feed.h>
#include <anerley/anerley-tp-item.h>
#include <anerley/anerley-econtact-item.h>
#include <anerley/anerley-presence-chooser.h>
#include <anerley/anerley-tp-user-avatar.h>

#include <libebook/e-book.h>

#include <meego-panel/mpl-panel-clutter.h>
#include <meego-panel/mpl-panel-common.h>
#include <meego-panel/mpl-entry.h>

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
};

static void _online_notify_cb (gboolean online,
                               gpointer userdata);

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
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", FALSE,
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
_messenger_launcher_button_clicked_cb (MxButton *button,
                                       gpointer  userdata)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (userdata);
  GAppInfo *app_info;
  GError *error = NULL;

  app_info = (GAppInfo *)g_desktop_app_info_new ("empathy.desktop");

  if (!g_app_info_launch (app_info,
                          NULL,
                          NULL,
                          &error))
  {
    g_warning (G_STRLOC ": Error starting empathy: %s", error->message);
    g_clear_error (&error);
  } else {
    if (priv->panel_client)
      mpl_panel_client_hide (priv->panel_client);
  }

  g_object_unref (app_info);
}

static ClutterActor *
_make_messenger_launcher_tile (MnbPeoplePanel *panel)
{
  ClutterActor *table;
  ClutterActor *icon_tex;
  ClutterActor *button;
  GAppInfo *app_info;
  gchar *button_str;
  ClutterActor *bin;

  bin = mx_frame_new ();
  clutter_actor_set_name (bin, "people-panel-messenger-launcher-tile");
  table = mx_table_new ();
  mx_bin_set_child (MX_BIN (bin), table);
  mx_table_set_column_spacing (MX_TABLE (table), 16);
  app_info = (GAppInfo *)g_desktop_app_info_new ("empathy.desktop");

  icon_tex = g_object_new (MX_TYPE_ICON,
                           "icon-name", "netbook-empathy",
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
                    (GCallback)_messenger_launcher_button_clicked_cb,
                    panel);

  mx_table_add_actor_with_properties (MX_TABLE (table),
                                      button,
                                      0, 1,
                                      "x-expand", FALSE,
                                      "y-expand", TRUE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      NULL);

  g_object_unref (app_info);

  return bin;
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
  gint accounts_available;

  g_object_get (priv->tp_feed,
                "accounts-available", &accounts_available,
                NULL);

  if (accounts_available == 0)
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
}

static void
_tp_feed_online_notify_cb (GObject    *object,
                           GParamSpec *pspec,
                           gpointer    userdata)
{
  _update_placeholder_state (MNB_PEOPLE_PANEL (userdata));
  _update_presence_chooser_state (MNB_PEOPLE_PANEL (userdata));
}

static void
_tp_feed_available_notify_cb (GObject    *object,
                              GParamSpec *pspec,
                              gpointer    userdata)
{
  _update_placeholder_state (MNB_PEOPLE_PANEL (userdata));
  _update_presence_chooser_state (MNB_PEOPLE_PANEL (userdata));
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

#if 0
static void
mnb_people_panel_init (MnbPeoplePanel *self)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);
  ClutterActor *label;
  ClutterActor *scroll_view, *scroll_bin, *bin, *tmp_text;
  AnerleyFeed *active_feed;

  mx_table_set_column_spacing (MX_TABLE (self), 4);
  mx_table_set_row_spacing (MX_TABLE (self), 6);
  clutter_actor_set_name (CLUTTER_ACTOR (self), "people-vbox");

  priv->header_box = mx_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (priv->header_box), "people-search");
  mx_table_set_column_spacing (MX_TABLE (priv->header_box), 20);
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      CLUTTER_ACTOR (priv->header_box),
                                      0, 0,
                                      "row-span", 1,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", TRUE,
                                      "x-align", MX_ALIGN_START,
                                      "y-align", MX_ALIGN_START,
                                      NULL);

  label = mx_label_new_with_text (_("People"));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "people-search-label");
  mx_table_add_actor_with_properties (MX_TABLE (priv->header_box),
                                      CLUTTER_ACTOR (label),
                                      0, 0,
                                      "x-expand", FALSE,
                                      "y-expand", FALSE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      "y-align", MX_ALIGN_MIDDLE,
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
                                      "x-align", MX_ALIGN_START,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      NULL);
  priv->presence_chooser = anerley_presence_chooser_new ();
  mx_table_add_actor_with_properties (MX_TABLE (priv->header_box),
                                      priv->presence_chooser,
                                      0, 2,
                                      "x-expand", TRUE,
                                      "x-fill", FALSE,
                                      "x-align", MX_ALIGN_END,
                                      "y-expand", TRUE,
                                      "y-fill", TRUE,
                                      NULL);
  g_signal_connect (priv->entry,
                    "text-changed",
                    (GCallback)_entry_text_changed_cb,
                    self);

  priv->tp_feed = anerley_aggregate_tp_feed_new ();

  priv->model = (AnerleyFeedModel *)anerley_feed_model_new (priv->tp_feed);
  priv->tile_view = anerley_tile_view_new (priv->model);

  active_feed = anerley_tp_monitor_feed_new ((AnerleyAggregateTpFeed *)priv->tp_feed,
                                             "MeegoPanelPeople");
  priv->active_model = (AnerleyFeedModel *)anerley_feed_model_new (active_feed);

  priv->active_list_view = anerley_compact_tile_view_new (priv->active_model);
  scroll_view = mx_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll_view),
                               priv->active_list_view);
  priv->content_table = mx_table_new ();
  clutter_actor_set_name (priv->content_table, "content-table");

  /* active conversations */
  priv->active_content_table = mx_table_new ();

  clutter_actor_hide (priv->active_content_table);
  clutter_actor_set_name (priv->active_content_table, "active-content-table");

  bin = mx_frame_new ();
  label = mx_label_new_with_text (_("You are chatting with:"));
  tmp_text = mx_label_get_clutter_text (MX_LABEL (label));
  clutter_text_set_ellipsize (CLUTTER_TEXT(tmp_text), PANGO_ELLIPSIZE_NONE);
  clutter_actor_set_name (label, "active-content-header-label");
  mx_bin_set_child (MX_BIN (bin), label);
  mx_bin_set_alignment (MX_BIN (bin), MX_ALIGN_START, MX_ALIGN_MIDDLE);
  mx_bin_set_fill (MX_BIN (bin), TRUE, TRUE);
  clutter_actor_set_name (bin, "active-content-header");

  mx_table_add_actor_with_properties (MX_TABLE (priv->active_content_table),
                                      bin,
                                      0, 0,
                                      "y-expand", FALSE,
                                      NULL);

  mx_table_add_actor (MX_TABLE (priv->active_content_table),
                      scroll_view,
                      1, 0);

  mx_table_add_actor_with_properties (MX_TABLE (priv->content_table),
                                      priv->active_content_table,
                                      0, 1,
                                      "x-expand", FALSE,
                                      NULL);


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
                      0,
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
                                      "y-align", MX_ALIGN_START,
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
                                      "y-align", MX_ALIGN_START,
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
                                      "y-align", MX_ALIGN_START,
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

  g_signal_connect (priv->active_list_view,
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

  sw_online_add_notify (_online_notify_cb, self);
  _update_online_state (self, sw_is_online ());
}
#endif

static void
mnb_people_panel_init (MnbPeoplePanel *self)
{
  MnbPeoplePanelPrivate *priv = GET_PRIVATE (self);
  ClutterActor *label;
  ClutterActor *scroll_view, *bin, *tmp_text;
  AnerleyFeed *active_feed;
  ClutterActor *settings_launcher;

  mx_table_set_column_spacing (MX_TABLE (self), 6);
  mx_table_set_row_spacing (MX_TABLE (self), 6);

  /* Populate top level table */
  priv->header_box = mx_table_new ();
  clutter_actor_set_name (priv->header_box, "people-panel-header-box");
  mx_table_set_column_spacing (MX_TABLE (priv->header_box), 20);
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      CLUTTER_ACTOR (priv->header_box),
                                      0, 0,
                                      "column-span", 2,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", TRUE,
                                      "x-align", MX_ALIGN_START,
                                      "y-align", MX_ALIGN_START,
                                      NULL);

  priv->content_table = mx_table_new ();
  clutter_actor_set_name (priv->content_table, "people-panel-content-box");
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      CLUTTER_ACTOR (priv->content_table),
                                      1, 0,
                                      "x-expand", TRUE,
                                      "y-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-fill", TRUE,
                                      NULL);

  priv->side_table = mx_table_new ();
  clutter_actor_set_width (priv->side_table, 288);
  clutter_actor_set_name (priv->side_table, "people-panel-side-box");
  mx_table_add_actor_with_properties (MX_TABLE (self),
                                      CLUTTER_ACTOR (priv->side_table),
                                      1, 1,
                                      "x-expand", FALSE,
                                      "y-expand", TRUE,
                                      "x-fill", TRUE,
                                      "y-fill", TRUE,
                                      NULL);


  /* Populate header */
  label = mx_label_new_with_text (_("People"));
  mx_table_add_actor_with_properties (MX_TABLE (priv->header_box),
                                      CLUTTER_ACTOR (label),
                                      0, 0,
                                      "x-expand", FALSE,
                                      "y-expand", FALSE,
                                      "x-fill", FALSE,
                                      "y-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      NULL);
  clutter_actor_set_name (label, "people-panel-header-label");

  /* Main body table */

  bin = mx_frame_new ();
  clutter_actor_set_name (bin, "people-panel-content-header");
  label = mx_label_new_with_text (_("Me and my people"));
  clutter_actor_set_name (label, "people-panel-content-header-label");
  mx_bin_set_child (MX_BIN (bin), label);
  mx_bin_set_alignment (MX_BIN (bin), MX_ALIGN_START, MX_ALIGN_MIDDLE);
  mx_table_add_actor_with_properties (MX_TABLE (priv->content_table),
                                      CLUTTER_ACTOR (bin),
                                      0, 0,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      NULL);

  /* Me table containing avatar, name and presence chooser */
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
                                      "row-span", 2,
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
  priv->presence_chooser = anerley_presence_chooser_new ();
  mx_table_add_actor_with_properties (MX_TABLE (priv->me_table),
                                      priv->presence_chooser,
                                      1, 1,
                                      "x-expand", TRUE,
                                      "x-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      "y-expand", TRUE,
                                      "y-fill", TRUE,
                                      NULL);

  settings_launcher = _make_settings_launcher (self);
  mx_table_add_actor_with_properties (MX_TABLE (priv->me_table),
                                      settings_launcher,
                                      0, 2,
                                      "x-expand", TRUE,
                                      "x-fill", FALSE,
                                      "x-align", MX_ALIGN_END,
                                      "y-expand", TRUE,
                                      "y-fill", FALSE,
                                      "row-span", 2,
                                      NULL);

  mx_table_add_actor_with_properties (MX_TABLE (priv->content_table),
                                      CLUTTER_ACTOR (priv->me_table),
                                      1, 0,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      NULL);

  priv->tp_feed = anerley_aggregate_tp_feed_new ();

  priv->model = (AnerleyFeedModel *)anerley_feed_model_new (priv->tp_feed);
  priv->tile_view = anerley_tile_view_new (priv->model);

  priv->main_scroll_view = mx_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->main_scroll_view),
                               priv->tile_view);

  mx_table_add_actor (MX_TABLE (priv->content_table),
                      priv->main_scroll_view,
                      2,
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
  label = mx_label_new_with_text (_("My conversations"));
  clutter_actor_set_name (label, "people-panel-side-box-header-label");
  bin = mx_frame_new ();
  clutter_actor_set_name (bin, "people-panel-side-box-header");
  mx_bin_set_child (MX_BIN (bin), label);
  mx_bin_set_alignment (MX_BIN (bin), MX_ALIGN_START, MX_ALIGN_MIDDLE);
  mx_table_add_actor_with_properties (MX_TABLE (priv->side_table),
                                      CLUTTER_ACTOR (bin),
                                      0, 0,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", FALSE,
                                      "x-align", MX_ALIGN_START,
                                      "y-align", MX_ALIGN_MIDDLE,
                                      NULL);
  mx_table_add_actor_with_properties (MX_TABLE (priv->side_table),
                                      _make_messenger_launcher_tile (self),
                                      1, 0,
                                      "x-expand", TRUE,
                                      "y-expand", FALSE,
                                      "x-fill", TRUE,
                                      "y-fill", FALSE,
                                      NULL);


  active_feed = anerley_tp_monitor_feed_new ((AnerleyAggregateTpFeed *)priv->tp_feed,
                                             "MeegoPanelPeople");
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
                                      2, 0,
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

  g_signal_connect (priv->tp_feed,
                    "notify::accounts-online",
                    (GCallback)_tp_feed_online_notify_cb,
                    self);
  g_signal_connect (priv->tp_feed,
                    "notify::accounts-available",
                    (GCallback)_tp_feed_available_notify_cb,
                    self);

  sw_online_add_notify (_online_notify_cb, self);
  _update_online_state (self, sw_is_online ());
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
