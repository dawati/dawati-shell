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

#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <clutter/clutter.h>
#include <nbtk/nbtk.h>

#include <mojito-client/mojito-client.h>

#include <telepathy-glib/account-manager.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/util.h>

#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>

#include "mnb-web-status-row.h"
#include "mnb-im-status-row.h"

#define ICON_SIZE       48
#define PADDING         8
#define BORDER_WIDTH    4

typedef struct _MoblinStatusPanel
{
  ClutterActor *table;
  ClutterActor *box;
  ClutterActor *empty_web_bin;
  ClutterActor *empty_im_bin;
  ClutterActor *header_label;

  MojitoClient *mojito_client;
  TpAccountManager *account_manager;

  GSList *services;
  GSList *accounts;

  guint is_online : 1;

  guint n_web_available;
  guint n_im_available;

  TpConnectionPresenceType im_presence;
  gchar *im_status;
} MoblinStatusPanel;

typedef struct _ServiceInfo
{
  gchar *name;

  MoblinStatusPanel *panel;

  MojitoClientService *service;
  ClutterActor *row;
  ClutterActor *box;

  guint caps_id;

  guint can_update : 1;
  guint has_icon   : 1;
  guint is_visible : 1;
} ServiceInfo;

typedef struct _AccountInfo
{
  gchar *name;

  MoblinStatusPanel *panel;

  TpAccount *account;
  ClutterActor *row;
  ClutterActor *box;

  guint is_visible : 1;
  guint is_enabled : 1;
} AccountInfo;

static void
account_info_destroy (gpointer data)
{
  if (G_LIKELY (data != NULL))
    {
      AccountInfo *a_info = data;

      g_free (a_info->name);
      g_object_unref (a_info->account);

      g_slice_free (AccountInfo, a_info);
    }
}

static void
service_info_destroy (gpointer data)
{
  if (G_LIKELY (data != NULL))
    {
      ServiceInfo *s_info = data;

      g_signal_handler_disconnect (s_info->service, s_info->caps_id);
      g_free (s_info->name);
      g_object_unref (s_info->service);

      g_slice_free (ServiceInfo, s_info);
    }
}

static AccountInfo *
account_find_by_name (MoblinStatusPanel *panel,
                      const gchar       *name)
{
  GSList *l;

  for (l = panel->accounts; l != NULL; l = l->next)
    {
      AccountInfo *a_info = l->data;

      if (strcmp (a_info->name, name) == 0)
        return a_info;
    }

  return NULL;
}

static void
on_row_status_changed (MnbIMStatusRow           *row,
                       TpConnectionPresenceType  presence,
                       const gchar              *status,
                       AccountInfo              *a_info)
{
  tp_account_manager_set_all_requested_presences (a_info->panel->account_manager,
                                                  presence, status, NULL);
}

static void
add_account_row (MoblinStatusPanel *panel,
                 AccountInfo       *a_info)
{
  a_info->row = g_object_new (MNB_TYPE_IM_STATUS_ROW,
                              "account-name", a_info->name,
                              NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (a_info->box), a_info->row);

  g_signal_connect (a_info->row, "status-changed",
                    G_CALLBACK (on_row_status_changed),
                    a_info);
}

static void
remove_account_row (MoblinStatusPanel *panel,
                    AccountInfo       *a_info)
{
  g_signal_handlers_disconnect_by_func (a_info->row, on_row_status_changed,
                                        a_info);

  clutter_container_remove_actor (CLUTTER_CONTAINER (a_info->box), a_info->row);
}

static void
on_presence_changed (TpAccountManager         *account_manager,
                     TpConnectionPresenceType  state,
                     const gchar              *status,
                     const gchar              *status_message,
                     MoblinStatusPanel        *panel)
{
  const gchar *state_str = NULL;
  GSList *a;

  panel->im_presence = state;

  switch (panel->im_presence)
    {
    case TP_CONNECTION_PRESENCE_TYPE_OFFLINE:
      state_str = "offline";
      break;

    case TP_CONNECTION_PRESENCE_TYPE_UNSET:
      state_str = "unset";
      break;

    case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE:
      state_str = "available";
      break;

    case TP_CONNECTION_PRESENCE_TYPE_AWAY:
      state_str = "away";
      break;

    case TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY:
      state_str = "extended away";
      break;

    case TP_CONNECTION_PRESENCE_TYPE_HIDDEN:
      state_str = "hidden";
      break;

    case TP_CONNECTION_PRESENCE_TYPE_BUSY:
      state_str = "busy";
      break;

    default:
      state_str = "unknown";
      break;
    }

  g_free (panel->im_status);
  panel->im_status = ((status_message != NULL && *status_message != '\0') ? g_strdup (status_message)
                                                                          : NULL);

  g_debug ("%s: PresenceChanged ['%s'[%d], '%s']",
           G_STRLOC,
           state_str,
           panel->im_presence,
           panel->im_status);

  for (a = panel->accounts; a != NULL; a = a->next)
    {
      AccountInfo *a_info = a->data;

      if (panel->is_online && a_info->is_enabled)
        {
          g_assert (a_info->row != NULL);

          clutter_actor_show (a_info->row);
          a_info->is_visible = TRUE;
        }
      else
        {
          g_assert (a_info->row != NULL);

          clutter_actor_hide (a_info->row);
          a_info->is_visible = FALSE;
        }

      g_debug ("setting status: '%s'", panel->im_status);
      mnb_im_status_row_set_status (MNB_IM_STATUS_ROW (a_info->row),
                                    panel->im_presence,
                                    panel->im_status);
    }
}

static void
on_caps_changed (MojitoClientService  *service,
                 const gchar         **new_caps,
                 ServiceInfo          *s_info)
{
  MoblinStatusPanel *panel = s_info->panel;
  gboolean was_visible, has_row;
  gboolean can_update, has_icon;

  was_visible = s_info->is_visible;
  has_row     = s_info->row != NULL;
  can_update  = FALSE;
  has_icon    = FALSE;

  while (*new_caps)
    {
      if (g_strcmp0 (*new_caps, CAN_UPDATE_STATUS) == 0)
        {
          can_update = TRUE;
          goto next_cap;
        }

      if (g_strcmp0 (*new_caps, CAN_REQUEST_AVATAR) == 0)
        {
          has_icon = TRUE;
          goto next_cap;
        }

    next_cap:
      new_caps++;
    }

  s_info->can_update = can_update;
  s_info->has_icon = has_icon;

  g_debug ("%s: CapabilitiesChanged['%s']: can-update:%s, has-icon:%s",
           G_STRLOC,
           s_info->name,
           s_info->can_update ? "yes" : "no",
           s_info->has_icon ? "yes" : "no");

  if (s_info->can_update && s_info->has_icon)
    {
      /* the caps we care about haven't changed */
      if (has_row && was_visible)
        return;

      if (!has_row)
        {
          s_info->row = g_object_new (MNB_TYPE_WEB_STATUS_ROW,
                                      "service-name", s_info->name,
                                      NULL);
          clutter_container_add_actor (CLUTTER_CONTAINER (s_info->box), s_info->row);
        }

      if (!was_visible)
        {
          clutter_actor_show (s_info->row);
          s_info->is_visible = TRUE;

          panel->n_web_available += 1;
        }
    }
  else
    {
      if (!has_row)
        return;

      if (was_visible)
        {
          clutter_actor_hide (s_info->row);
          s_info->is_visible = FALSE;

          panel->n_web_available -= 1;
        }
    }

  if (panel->n_web_available == 0)
    clutter_actor_show (panel->empty_web_bin);
  else
    clutter_actor_hide (panel->empty_web_bin);
}

static void
get_dynamic_caps (MojitoClientService  *service,
                  const gchar         **caps,
                  const GError         *error,
                  gpointer              user_data)
{
  ServiceInfo *s_info = user_data;

  if (error)
    {
      g_critical ("Unable to retrieve dynamic caps for service '%s': %s",
                  s_info->name,
                  error->message);
      return;
    }

  on_caps_changed (service, caps, s_info);
}

static void
get_static_caps (MojitoClientService  *service,
                 const gchar         **caps,
                 const GError         *error,
                 gpointer              user_data)
{
  ServiceInfo *s_info = user_data;
  gboolean can_update_status, can_request_avatar;

  if (error)
    {
      g_critical ("Unable to retrieve static caps for service '%s': %s",
                  s_info->name,
                  error->message);
      return;
    }

  can_update_status = can_request_avatar = FALSE;
  while (*caps)
    {
      if (g_strcmp0 (*caps, CAN_UPDATE_STATUS) == 0)
        can_update_status = TRUE;

      if (g_strcmp0 (*caps, CAN_REQUEST_AVATAR) == 0)
        can_request_avatar = TRUE;

      caps++;
    }

  if (!can_update_status || !can_request_avatar)
    return;

  s_info->row = g_object_new (MNB_TYPE_WEB_STATUS_ROW,
                              "service-name", s_info->name,
                              NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (s_info->box), s_info->row);

  clutter_actor_hide (s_info->row);
  s_info->is_visible = FALSE;

  s_info->caps_id =
    g_signal_connect (s_info->service, "capabilities-changed",
                      G_CALLBACK (on_caps_changed),
                      s_info);

  mojito_client_service_get_dynamic_capabilities (s_info->service,
                                                  get_dynamic_caps,
                                                  s_info);
}

static void
on_mojito_get_services (MojitoClient *client,
                        const GList  *services,
                        gpointer      data)
{
  MoblinStatusPanel *panel = data;
  GSList *new_services;
  const GList *s;

  new_services = NULL;
  for (s = services; s != NULL; s = s->next)
    {
      const gchar *service_name = s->data;
      MojitoClientService *service;
      ServiceInfo *s_info;

      if (service_name == NULL || *service_name == '\0')
        break;

      /* filter out the dummy service */
      if (strcmp (service_name, "dummy") == 0)
        continue;

      service = mojito_client_get_service (client, service_name);
      if (G_UNLIKELY (service == NULL))
        continue;

      s_info = g_slice_new (ServiceInfo);
      s_info->panel = panel;
      s_info->name = g_strdup (service_name);
      s_info->service = service;
      s_info->row = NULL;
      s_info->box = panel->box;
      s_info->can_update = FALSE;
      s_info->has_icon = FALSE;
      s_info->is_visible = FALSE;

      mojito_client_service_get_static_capabilities (s_info->service,
                                                     get_static_caps,
                                                     s_info);

      new_services = g_slist_prepend (new_services, s_info);
    }

  g_slist_foreach (panel->services, (GFunc) service_info_destroy, NULL);
  g_slist_free (panel->services);

  panel->services = g_slist_reverse (new_services);
}

static void
update_header (NbtkLabel *header,
               gboolean   is_online)
{
  if (is_online)
    nbtk_label_set_text (header, _("Your current status"));
  else
    nbtk_label_set_text (header, _("Your current status - you are offline"));
}

static void
update_im_status (MoblinStatusPanel *panel,
                  gboolean           is_online)
{
  TpConnectionPresenceType cur_state, actual_state = TP_CONNECTION_PRESENCE_TYPE_UNSET;
  gchar *actual_status = NULL;
  const gchar *state_str = NULL;
  GError *error = NULL;

  cur_state = panel->im_presence;

  actual_state = tp_account_manager_get_most_available_presence (panel->account_manager,
                                                                 NULL, &actual_status);

  switch (cur_state)
    {
    case TP_CONNECTION_PRESENCE_TYPE_OFFLINE:
      state_str = _("Offline");
      break;

    case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE:
      state_str = _("Available");
      break;

    case TP_CONNECTION_PRESENCE_TYPE_AWAY:
      state_str = _("Away");
      break;

    case TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY:
      state_str = _("Away");
      break;

    case TP_CONNECTION_PRESENCE_TYPE_HIDDEN:
      state_str = _("Busy");
      break;

    case TP_CONNECTION_PRESENCE_TYPE_BUSY:
      state_str = _("Busy");
      break;

    default:
      break;
    }

  /* if we are not online, don't bother */
  if (!is_online)
    return;

  g_debug ("%s: cur_state [%d], mc_state [%d]",
           G_STRLOC,
           cur_state,
           actual_state);

  if (cur_state == TP_CONNECTION_PRESENCE_TYPE_UNSET || cur_state == actual_state)
    {
      on_presence_changed (panel->account_manager,
                           panel->im_presence,
                           state_str, NULL,
                           panel);
    }
  else if (cur_state != actual_state && cur_state == TP_CONNECTION_PRESENCE_TYPE_AVAILABLE)
    {
      tp_account_manager_set_all_requested_presences (panel->account_manager,
                                                      cur_state, NULL, state_str);
    }
  else
    {
      tp_account_manager_set_all_requested_presences (panel->account_manager,
                                                      actual_state,
                                                      NULL, actual_status);
    }

  g_free (actual_status);
}

static void
on_account_ready (GObject *source_object,
                  GAsyncResult *result,
                  gpointer user_data)
{
  MoblinStatusPanel *panel = (MoblinStatusPanel *) user_data;
  AccountInfo *a_info;
  const gchar *name;
  TpAccount *account = TP_ACCOUNT (source_object);

  if (!tp_account_prepare_finish (account, result, NULL))
    return;

  name = tp_proxy_get_object_path (account);

  a_info = account_find_by_name (panel, name);
  if (a_info == NULL)
    {
      a_info = g_slice_new (AccountInfo);
      a_info->name = g_strdup (name);
      a_info->panel = panel;
      a_info->account = g_object_ref (account);
      a_info->row = NULL;
      a_info->box = panel->box;
      a_info->is_visible = FALSE;
      a_info->is_enabled = TRUE;

      add_account_row (panel, a_info);

      panel->accounts = g_slist_prepend (panel->accounts, a_info);
    }
  else
    a_info->is_enabled = TRUE;

  panel->n_im_available += 1;

  clutter_actor_hide (panel->empty_im_bin);

  mnb_im_status_row_set_online (MNB_IM_STATUS_ROW (a_info->row),
                                panel->is_online);

  update_im_status (panel, panel->is_online);
}

static void
on_account_enabled (TpAccountManager  *account_manager,
                    TpAccount         *account,
                    MoblinStatusPanel *panel)
{
  tp_account_prepare_async (account, NULL, on_account_ready, panel);
}

static void
on_account_disabled (TpAccountManager  *account_manager,
                     TpAccount         *account,
                     MoblinStatusPanel *panel)
{
  AccountInfo *a_info;
  const gchar *name;

  name = tp_proxy_get_object_path (account);

  a_info = account_find_by_name (panel, name);
  if (a_info == NULL)
    return;

  a_info->is_enabled = FALSE;

  panel->n_im_available -= 1;

  if (panel->n_im_available == 0)
    clutter_actor_show (panel->empty_im_bin);

  update_im_status (panel, panel->is_online);
}

static void
on_account_removed (TpAccountManager *manager,
                    TpAccount        *account,
                    gpointer          user_data)
{
  MoblinStatusPanel *panel = (MoblinStatusPanel *) user_data;
  AccountInfo *a_info;
  const gchar *name;

  name = tp_proxy_get_object_path (account);

  a_info = account_find_by_name (panel, name);
  if (a_info == NULL)
    return;

  on_account_disabled (manager, account, panel);

  /* We need to actually remove it from the accounts list because
   * the object path might be re-used in the future. */
  remove_account_row (panel, a_info);

  panel->accounts = g_slist_remove (panel->accounts, a_info);
  account_info_destroy (a_info);
}

static void
on_mojito_online_changed (MojitoClient      *client,
                          gboolean           is_online,
                          MoblinStatusPanel *panel)
{
  GList *accounts, *l;
  GSList *cur_accounts, *a;

  g_debug ("%s: We are now %s", G_STRLOC, is_online ? "online" : "offline");

  panel->is_online = is_online;

  cur_accounts = NULL;
  accounts = tp_account_manager_get_valid_accounts (panel->account_manager);
  for (l = accounts; l != NULL; l = l->next)
    {
      TpAccount *account = l->data;
      AccountInfo *a_info;
      const gchar *name;

      if (!tp_account_is_enabled (account))
        continue;

      name = tp_proxy_get_object_path (account);
      if (account_find_by_name (panel, name) != NULL)
        continue;

      a_info = g_slice_new (AccountInfo);
      a_info->name = g_strdup (name);
      a_info->panel = panel;
      a_info->account = g_object_ref (account);
      a_info->row = NULL;
      a_info->box = panel->box;
      a_info->is_visible = FALSE;
      a_info->is_enabled = TRUE;

      add_account_row (panel, a_info);

      cur_accounts = g_slist_prepend (cur_accounts, a_info);
    }

  g_list_free (accounts);

  if (panel->accounts == NULL)
    panel->accounts = g_slist_reverse (cur_accounts);
  else
    panel->accounts->next = g_slist_reverse (cur_accounts);

  panel->n_im_available = g_slist_length (panel->accounts);
  if (panel->n_im_available == 0)
    clutter_actor_show (panel->empty_im_bin);
  else
    clutter_actor_hide (panel->empty_im_bin);

  for (a = panel->accounts; a != NULL; a = a->next)
    {
      AccountInfo *a_info = a->data;

      mnb_im_status_row_set_online (MNB_IM_STATUS_ROW (a_info->row),
                                    panel->is_online);
    }

  update_header (NBTK_LABEL (panel->header_label), is_online);

  update_im_status (panel, is_online);

}

static void
on_account_manager_ready (GObject      *source_object,
                          GAsyncResult *result,
                          gpointer      userdata)
{
  TpAccountManager *account_manager = TP_ACCOUNT_MANAGER (source_object);
  GList *accounts, *l;
  GSList *cur_accounts, *a;
  MoblinStatusPanel *panel = userdata;
  GError *error = NULL;

  if (!tp_account_manager_prepare_finish (account_manager, result, &error))
  {
    g_warning ("Failed to prepare account manager: %s", error->message);
    g_error_free (error);
    return;
  }

  g_debug ("%s: Account manager is ready\n", G_STRLOC);

  /* Get initial presence */
  g_free (panel->im_status);
  panel->im_presence = tp_account_manager_get_most_available_presence (account_manager, NULL,
                                                                       &(panel->im_status));

  if (panel->is_online)
    {
      panel->im_presence = TP_CONNECTION_PRESENCE_TYPE_AVAILABLE;
      g_free (panel->im_status);
      panel->im_status = NULL;
    }

  cur_accounts = NULL;
  accounts = tp_account_manager_get_valid_accounts (panel->account_manager);
  for (l = accounts; l != NULL; l = l->next)
    {
      TpAccount *account = l->data;
      AccountInfo *a_info;
      const gchar *name;

      if (!tp_account_is_enabled (account))
        continue;

      name = tp_proxy_get_object_path (account);
      if (account_find_by_name (panel, name) != NULL)
        continue;

      a_info = g_slice_new (AccountInfo);
      a_info->name = g_strdup (name);
      a_info->panel = panel;
      a_info->account = g_object_ref (account);
      a_info->row = NULL;
      a_info->box = panel->box;
      a_info->is_visible = FALSE;
      a_info->is_enabled = TRUE;

      add_account_row (panel, a_info);

      cur_accounts = g_slist_prepend (cur_accounts, a_info);
    }

  g_list_free (accounts);

  if (panel->accounts == NULL)
    panel->accounts = g_slist_reverse (cur_accounts);
  else
    panel->accounts->next = g_slist_reverse (cur_accounts);

  panel->n_im_available = g_slist_length (panel->accounts);
  if (panel->n_im_available == 0)
    clutter_actor_show (panel->empty_im_bin);
  else
    clutter_actor_hide (panel->empty_im_bin);

  for (a = panel->accounts; a != NULL; a = a->next)
    {
      AccountInfo *a_info = a->data;

      mnb_im_status_row_set_online (MNB_IM_STATUS_ROW (a_info->row),
                                    panel->is_online);
    }

  update_header (NBTK_LABEL (panel->header_label), panel->is_online);
  update_im_status (panel, panel->is_online);

  /* get notification when the online state changes */
  g_signal_connect (panel->mojito_client, "online-changed",
                    G_CALLBACK (on_mojito_online_changed),
                    panel);

}

static void
on_mojito_is_online (MojitoClient *client,
                     gboolean      is_online,
                     gpointer      data)
{
  MoblinStatusPanel *panel = data;

  g_debug ("%s: We are now %s", G_STRLOC, is_online ? "online" : "offline");

  panel->is_online = is_online;

  tp_account_manager_prepare_async (panel->account_manager, NULL,
                                    on_account_manager_ready, panel);
}

#if 0
static ClutterActor *
make_empty_status_tile (gint width)
{
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

  return tile;
}
#endif

static ClutterActor *
make_status (MoblinStatusPanel *panel)
{
  ClutterActor *table;
  NbtkWidget *header, *label, *scroll;

  table = CLUTTER_ACTOR (nbtk_table_new ());
  nbtk_widget_set_style_class_name (NBTK_WIDGET (table), "MnbStatusPageTable");
  nbtk_table_set_row_spacing (NBTK_TABLE (table), 6);
  clutter_actor_set_reactive (table, TRUE);
  panel->table = table;

  header = nbtk_label_new (_("Your current status - you are offline"));
  nbtk_widget_set_style_class_name (header, "MnbStatusPageHeader");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (table),
                                        CLUTTER_ACTOR (header),
                                        0, 0,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);
  panel->header_label = CLUTTER_ACTOR (header);

  panel->empty_web_bin = CLUTTER_ACTOR (nbtk_bin_new ());
  nbtk_widget_set_style_class_name (NBTK_WIDGET (panel->empty_web_bin),
                                    "status-web-empty-bin");
  nbtk_bin_set_alignment (NBTK_BIN (panel->empty_web_bin),
                          NBTK_ALIGN_START,
                          NBTK_ALIGN_MIDDLE);
  nbtk_bin_set_fill (NBTK_BIN (panel->empty_web_bin), TRUE, FALSE);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (table), panel->empty_web_bin,
                                        1, 0,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        "row-span", 1,
                                        "col-span", 1,
                                        "allocate-hidden", FALSE,
                                        NULL);

  label = nbtk_label_new (_("To update your web status you need to setup "
                            "a Web Services account with a provider that "
                            "supports status messages"));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (label), "status-web-empty-label");
  clutter_container_add_actor (CLUTTER_CONTAINER (panel->empty_web_bin),
                               CLUTTER_ACTOR (label));

  panel->empty_im_bin = CLUTTER_ACTOR (nbtk_bin_new ());
  nbtk_widget_set_style_class_name (NBTK_WIDGET (panel->empty_im_bin),
                                    "status-im-empty-bin");
  nbtk_bin_set_alignment (NBTK_BIN (panel->empty_im_bin),
                          NBTK_ALIGN_START,
                          NBTK_ALIGN_MIDDLE);
  nbtk_bin_set_fill (NBTK_BIN (panel->empty_im_bin), TRUE, FALSE);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (table), panel->empty_im_bin,
                                        2, 0,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        "row-span", 1,
                                        "col-span", 1,
                                        "allocate-hidden", FALSE,
                                        NULL);

  label = nbtk_label_new (_("To update your IM status you need to setup "
                            "an Instant Messaging account"));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (label), "status-im-empty-label");
  clutter_container_add_actor (CLUTTER_CONTAINER (panel->empty_im_bin),
                               CLUTTER_ACTOR (label));

  scroll = nbtk_scroll_view_new ();
  nbtk_table_add_actor_with_properties (NBTK_TABLE (table),
                                        CLUTTER_ACTOR (scroll),
                                        3, 0,
                                        "x-expand", TRUE,
                                        "y-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        "row-span", 1,
                                        "col-span", 1,
                                        "allocate-hidden", FALSE,
                                        NULL);

  panel->box = CLUTTER_ACTOR (nbtk_box_layout_new ());
  nbtk_box_layout_set_vertical (NBTK_BOX_LAYOUT (panel->box), TRUE);
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll), panel->box);

  /*
   * we need an account manager first because the online/offline notification
   * is managed through Mojito; XXX yes, this is dumb - Moblin needs
   * a "shell" library that notifies when we're online or offline
   */
  panel->account_manager = tp_account_manager_dup ();
  panel->im_presence = TP_CONNECTION_PRESENCE_TYPE_UNSET;
  panel->im_status = NULL;

  g_signal_connect (panel->account_manager,
                    "account-enabled", G_CALLBACK (on_account_enabled),
                    panel);
  g_signal_connect (panel->account_manager,
                    "account-disabled", G_CALLBACK (on_account_disabled),
                    panel);
  g_signal_connect (panel->account_manager,
                    "account-removed", G_CALLBACK (on_account_removed),
                    panel);
  g_signal_connect (panel->account_manager,
                    "most-available-presence-changed",
                    G_CALLBACK (on_presence_changed), panel);

  /* mojito: web services */
  panel->mojito_client = mojito_client_new ();

  /* online notification on the header */
  mojito_client_is_online (panel->mojito_client, on_mojito_is_online, panel);

  /* start retrieving the services */
  mojito_client_get_services (panel->mojito_client,
                              on_mojito_get_services,
                              panel);

  return table;
}

static void
on_client_set_size (MplPanelClient *client,
                    guint           width,
                    guint           height,
                    ClutterActor   *table)
{
  clutter_actor_set_size (table, width, height);
}

static void
setup_standalone (MoblinStatusPanel *status_panel)
{
  ClutterActor *stage, *status;

  status = make_status (status_panel);
  clutter_actor_set_size (status, 1000, 400);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 1000, 400);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), status);

  clutter_actor_show (stage);
}

static void
setup_panel (MoblinStatusPanel *status_panel)
{
  MplPanelClient *panel;
  ClutterActor *stage, *status;

  panel = mpl_panel_clutter_new (MPL_PANEL_STATUS,
                                 _("status"),
                                 NULL,
                                 "status-button",
                                 TRUE);

  status = make_status (status_panel);
  mpl_panel_clutter_track_actor_height (MPL_PANEL_CLUTTER (panel), status);
  clutter_actor_set_height (status, 400);

#if 0
  g_signal_connect (panel,
                    "show-begin", G_CALLBACK (on_status_show_begin),
                    status);
  g_signal_connect (panel,
                    "show-end", G_CALLBACK (on_status_show_end),
                    status);
  g_signal_connect (panel,
                    "hide-end", G_CALLBACK (on_status_hide_end),
                    status);
#endif

  stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (panel));
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), status);

  g_signal_connect (panel,
                    "set-size", G_CALLBACK (on_client_set_size),
                    status);
}

static gboolean status_standalone = FALSE;

static GOptionEntry status_options[] = {
  {
    "standalone", 's',
    0,
    G_OPTION_ARG_NONE, &status_standalone,
    "Do not embed into mutter-moblin", NULL
  },

  { NULL }
};

int
main (int argc, char *argv[])
{
  MoblinStatusPanel *panel;
  GError *error;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  error = NULL;
  clutter_init_with_args (&argc, &argv,
                          "- Mutter-moblin's status page",
                          status_options,
                          GETTEXT_PACKAGE,
                          &error);

  if (error)
    {
      g_critical ("Unable to initialize Clutter: %s", error->message);
      g_clear_error (&error);
      return EXIT_FAILURE;
    }

  nbtk_texture_cache_load_cache (nbtk_texture_cache_get_default (),
                                 NBTK_CACHE);
  nbtk_style_load_from_file (nbtk_style_get_default (),
                             THEMEDIR "/panel.css",
                             &error);

  if (error)
    {
      g_critical ("Unbale to load theme: %s", error->message);
      g_clear_error (&error);
    }

  panel = g_new0 (MoblinStatusPanel, 1);

  if (status_standalone)
    setup_standalone (panel);
  else
    setup_panel (panel);

  clutter_main ();

  /* clean up */
  g_slist_foreach (panel->accounts, (GFunc) account_info_destroy, NULL);
  g_slist_free (panel->accounts);

  g_slist_foreach (panel->services, (GFunc) service_info_destroy, NULL);
  g_slist_free (panel->services);

  g_object_unref (panel->account_manager);
  g_object_unref (panel->mojito_client);
  g_free (panel->im_status);
  g_free (panel);

  return EXIT_SUCCESS;
}

/* vim:set ts=8 expandtab */
