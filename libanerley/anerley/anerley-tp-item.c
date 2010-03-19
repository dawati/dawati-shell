/*
 * Anerley - people feeds and widgets
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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
 *
 */

#include <telepathy-glib/telepathy-glib.h>

#include "anerley-tp-item.h"

G_DEFINE_TYPE (AnerleyTpItem, anerley_tp_item, ANERLEY_TYPE_ITEM)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_TP_ITEM, AnerleyTpItemPrivate))

#define GET_PRIVATE(o) \
    ((AnerleyTpItem *)o)->priv

struct _AnerleyTpItemPrivate {
  TpAccount *account;
  TpContact *contact;

  GSList *channel_requests;
  GSList *signal_connections;

  gchar *display_name;
  gchar *avatar_path;
  gchar *presence_message;
  gchar *sortable_name;

  const gchar *presence_status;

  GHashTable *pending_messages; /* reffed TpChannel* to GSList of guint */
};

enum
{
  PROP_0,
  PROP_CONTACT,
  PROP_ACCOUNT,
};

static void anerley_tp_item_update_contact (AnerleyTpItem *item,
                                            TpContact     *contact);

static void
anerley_tp_item_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_CONTACT:
      g_value_set_object (value, priv->contact);
      break;
    case PROP_ACCOUNT:
      g_value_set_object (value, priv->account);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tp_item_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_CONTACT:
      anerley_tp_item_update_contact ((AnerleyTpItem *)object, 
                                      (TpContact *)g_value_get_object (value));
      break;
    case PROP_ACCOUNT:
      priv->account = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tp_item_dispose (GObject *object)
{
  AnerleyTpItem *item = (AnerleyTpItem *)object;
  AnerleyTpItemPrivate *priv = GET_PRIVATE (object);

  anerley_tp_item_update_contact (item, NULL);

  if (priv->account)
  {
    g_object_unref (priv->account);
    priv->account = NULL;
  }

  if (priv->channel_requests != NULL)
  {
    g_slist_foreach (priv->channel_requests, (GFunc) g_object_unref, NULL);
    g_slist_free (priv->channel_requests);
    priv->channel_requests = NULL;
  }

  if (priv->signal_connections != NULL)
  {
    g_slist_foreach (priv->signal_connections,
                     (GFunc) tp_proxy_signal_connection_disconnect, NULL);
    g_slist_free (priv->signal_connections);
    priv->signal_connections = NULL;
  }

  if (priv->pending_messages)
    {
      GHashTableIter iter;
      gpointer key, value;

      g_hash_table_iter_init (&iter, priv->pending_messages);
      while (g_hash_table_iter_next (&iter, &key, &value))
        {
          g_object_unref (key);
          g_slist_free (value);
        }

      g_hash_table_destroy (priv->pending_messages);
      priv->pending_messages = NULL;
    }

  G_OBJECT_CLASS (anerley_tp_item_parent_class)->dispose (object);
}

static void
anerley_tp_item_finalize (GObject *object)
{
  G_OBJECT_CLASS (anerley_tp_item_parent_class)->finalize (object);
}

static const gchar *
anerley_tp_item_get_display_name (AnerleyItem *item)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);

  return priv->display_name;
}

static const gchar *
anerley_tp_item_get_sortable_name (AnerleyItem *item)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);

  return priv->sortable_name;
}

static const gchar *
anerley_tp_item_get_avatar_path (AnerleyItem *item)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);

  return priv->avatar_path;
}

static const gchar *
anerley_tp_item_get_presence_status (AnerleyItem *item)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);

  return priv->presence_status;
}

static const gchar *
anerley_tp_item_get_presence_message (AnerleyItem *item)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);

  return priv->presence_message;
}

static void
_item_activate_proceed_cb (TpChannelRequest *proxy,
                           const GError     *error,
                           gpointer          userdata,
                           GObject          *weak_object)
{
  if (error != NULL)
    g_warning ("Failed to call Proceed on the channel request: %s",
               error->message);
}

static void
_item_activate_failed_cb (TpChannelRequest *proxy,
                          const gchar      *arg_Error,
                          const gchar      *arg_Message,
                          gpointer          userdata,
                          GObject          *weak_object)
{
  if (arg_Error != NULL)
  {
    g_warning ("Channel request failed: %s: %s", arg_Error, arg_Message);
    return;
  }
}

static void
_item_activate_succeeded_cb (TpChannelRequest *proxy,
                             gpointer          userdata,
                             GObject          *weak_object)
{
  g_debug ("Channel request succeeded\n");
}

static void
_item_activate_ensure_channel_cb (TpChannelDispatcher *proxy,
                                  const gchar         *out_Request,
                                  const GError        *error,
                                  gpointer             userdata,
                                  GObject             *weak_object)
{
  TpChannelRequest *request;
  TpDBusDaemon *dbus;
  AnerleyTpItemPrivate *priv = GET_PRIVATE (weak_object);
  TpProxySignalConnection *signal;

  if (error != NULL)
  {
    g_warning ("Failed to CreateChannel: %s", error->message);
    return;
  }

  dbus = tp_dbus_daemon_dup (NULL);

  request = tp_channel_request_new (dbus, out_Request, NULL, NULL);

  /* Connect to Succeeded and add it to the list */
  signal = tp_cli_channel_request_connect_to_succeeded (request, _item_activate_succeeded_cb,
                                                        NULL, NULL, weak_object, NULL);

  priv->signal_connections = g_slist_append (priv->signal_connections, signal);

  /* Connect to Failed and add it to the list */
  signal = tp_cli_channel_request_connect_to_failed (request, _item_activate_failed_cb,
                                                     NULL, NULL, weak_object, NULL);

  priv->signal_connections = g_slist_append (priv->signal_connections, signal);

  /* Call Proceed and add the request to the list */
  tp_cli_channel_request_call_proceed (request, -1, _item_activate_proceed_cb,
                                       NULL, NULL, weak_object);

  priv->channel_requests = g_slist_append (priv->channel_requests, request);

  g_object_unref (dbus);
}

static guint
anerley_tp_item_get_unread_messages_count (AnerleyItem *item)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);
  GHashTableIter iter;
  gpointer key, value;
  guint total = 0;

  g_return_val_if_fail (ANERLEY_IS_TP_ITEM (item), 0);

  g_hash_table_iter_init (&iter, priv->pending_messages);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      total += g_slist_length (value);
    }

  return total;
}

static void
anerley_tp_item_activate (AnerleyItem *item)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);
  TpChannelDispatcher *dispatcher;
  TpDBusDaemon *dbus;
  GTimeVal t;
  GHashTable *properties;

  dbus = tp_dbus_daemon_dup (NULL);
  dispatcher = tp_channel_dispatcher_new (dbus);

  g_get_current_time (&t);

  properties = tp_asv_new (TP_IFACE_CHANNEL ".ChannelType",
                           G_TYPE_STRING,
                           TP_IFACE_CHANNEL_TYPE_TEXT,

                           TP_IFACE_CHANNEL ".TargetHandleType",
                           G_TYPE_UINT,
                           TP_HANDLE_TYPE_CONTACT,

                           TP_IFACE_CHANNEL ".TargetHandle",
                           G_TYPE_UINT,
                           tp_contact_get_handle (priv->contact),

                           NULL);

  tp_cli_channel_dispatcher_call_ensure_channel (dispatcher,
                                                 -1,
                                                 tp_proxy_get_object_path (priv->account),
                                                 properties,
                                                 t.tv_sec,
                                                 "",
                                                 _item_activate_ensure_channel_cb,
                                                 NULL,
                                                 NULL,
                                                 (GObject *)item);

  g_hash_table_destroy (properties);
  g_object_unref (dispatcher);
  g_object_unref (dbus);
}

static void
anerley_tp_item_class_init (AnerleyTpItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  AnerleyItemClass *item_class = ANERLEY_ITEM_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyTpItemPrivate));

  object_class->get_property = anerley_tp_item_get_property;
  object_class->set_property = anerley_tp_item_set_property;
  object_class->dispose = anerley_tp_item_dispose;
  object_class->finalize = anerley_tp_item_finalize;

  item_class->get_display_name = anerley_tp_item_get_display_name;
  item_class->get_sortable_name = anerley_tp_item_get_sortable_name;
  item_class->get_avatar_path = anerley_tp_item_get_avatar_path;
  item_class->get_presence_status = anerley_tp_item_get_presence_status;
  item_class->get_presence_message = anerley_tp_item_get_presence_message;
  item_class->get_unread_messages_count = anerley_tp_item_get_unread_messages_count;
  item_class->activate = anerley_tp_item_activate;

  pspec = g_param_spec_object ("account",
                               "The account",
                               "The mission control account",
                               TP_TYPE_ACCOUNT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_ACCOUNT, pspec);

  pspec = g_param_spec_object ("contact",
                               "The contact",
                               "Underlying contact whose details we represent",
                               TP_TYPE_CONTACT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_CONTACT, pspec);
}

static void
anerley_tp_item_init (AnerleyTpItem *self)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE_REAL (self);
  self->priv = priv;

  priv->channel_requests = NULL;
  priv->signal_connections = NULL;

  priv->pending_messages = g_hash_table_new (NULL, NULL);
}

AnerleyTpItem *
anerley_tp_item_new (TpAccount *account,
                     TpContact *contact)
{
  return g_object_new (ANERLEY_TYPE_TP_ITEM,
                       "account",
                       account,
                       "contact",
                       contact,
                       NULL);
}

static void
_contact_notify_alias_cb (GObject    *object,
                          GParamSpec *pspec,
                          gpointer    userdata)
{
  TpContact *contact = (TpContact *)object;
  AnerleyTpItem *item = (AnerleyTpItem *)userdata;
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);
  const gchar *alias;

  g_free (priv->display_name);
  priv->display_name = NULL;
  g_free (priv->sortable_name);
  priv->sortable_name = NULL;

  alias = tp_contact_get_alias (contact);

  if (alias)
  {
    priv->display_name = g_strdup (alias);
    priv->sortable_name = g_utf8_casefold (alias, -1);
  }

  anerley_item_emit_display_name_changed ((AnerleyItem *)item);
}

static const gchar *
_get_real_presence_status (TpContact *contact)
{
  switch (tp_contact_get_presence_type (contact))
  {
    case TP_CONNECTION_PRESENCE_TYPE_UNSET:
      return "offline";
    case TP_CONNECTION_PRESENCE_TYPE_OFFLINE:
      return "offline";
    case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE:
      return "available";
    case TP_CONNECTION_PRESENCE_TYPE_AWAY:
      return "away";
    case TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY:
      return "xa";
    case TP_CONNECTION_PRESENCE_TYPE_HIDDEN:
      return "offline";
    case TP_CONNECTION_PRESENCE_TYPE_BUSY:
      return "dnd";
    case TP_CONNECTION_PRESENCE_TYPE_UNKNOWN:
      return "offline";
    case TP_CONNECTION_PRESENCE_TYPE_ERROR:
      return "offline";
    default:
      return "offline";
  }
}

static void
_contact_notify_presence_status_cb (GObject    *object,
                                    GParamSpec *pspec,
                                    gpointer    userdata)
{
  TpContact *contact = (TpContact *)object;
  AnerleyTpItem *item = (AnerleyTpItem *)userdata;
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);

  if (_get_real_presence_status (contact))
  {
    priv->presence_status = _get_real_presence_status (contact);
  } else {
    priv->presence_status = NULL;
  }

  anerley_item_emit_presence_changed ((AnerleyItem *)item);
}

static void
_contact_notify_presence_message_cb (GObject    *object,
                                     GParamSpec *pspec,
                                     gpointer    userdata)
{
  TpContact *contact = (TpContact *)object;
  AnerleyTpItem *item = (AnerleyTpItem *)userdata;
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);

  g_free (priv->presence_message);
  priv->presence_message = NULL;

  if (tp_contact_get_presence_message (contact))
  {
    priv->presence_message = g_strdup (tp_contact_get_presence_message (contact));
  }

  anerley_item_emit_presence_changed ((AnerleyItem *)item);
}


static void
anerley_tp_item_update_contact (AnerleyTpItem *item,
                                TpContact     *contact)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);

  if (priv->contact == contact)
    return;

  if (priv->contact)
  {
    g_signal_handlers_disconnect_by_func (priv->contact,
                                          _contact_notify_alias_cb,
                                          item);
    g_signal_handlers_disconnect_by_func (priv->contact,
                                          _contact_notify_presence_status_cb,
                                          item);
    g_signal_handlers_disconnect_by_func (priv->contact,
                                          _contact_notify_presence_message_cb,
                                          item);
    g_object_unref (priv->contact);
  }

  g_free (priv->display_name);
  g_free (priv->avatar_path);
  g_free (priv->presence_message);

  priv->display_name = NULL;
  priv->avatar_path = NULL;
  priv->presence_status = NULL;
  priv->presence_message = NULL;

  if (contact)
  {
    priv->contact = g_object_ref (contact);
    g_signal_connect (priv->contact,
                      "notify::alias",
                      (GCallback)_contact_notify_alias_cb,
                      item);
    g_signal_connect (priv->contact,
                      "notify::presence-status",
                      (GCallback)_contact_notify_presence_status_cb,
                      item);
    g_signal_connect (priv->contact,
                      "notify::presence-message",
                      (GCallback)_contact_notify_presence_message_cb,
                      item);

    /* Simulate the hook-ups */
    g_object_notify ((GObject *)priv->contact,
                     "alias");
    g_object_notify ((GObject *)priv->contact,
                     "presence-status");
    g_object_notify ((GObject *)priv->contact,
                     "presence-message");
  }
}

void
anerley_tp_item_set_avatar_path (AnerleyTpItem *item,
                                 const gchar    *avatar_path)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);

  g_free (priv->avatar_path);
  priv->avatar_path = g_strdup (avatar_path);

  anerley_item_emit_avatar_path_changed ((AnerleyItem *)item);
}

static void
_message_received (TpChannel  *channel,
                   guint       id,
                   guint       timestamp,
                   guint       sender,
                   guint       type,
                   guint       flags,
                   const char *text,
                   gpointer    user_data,
                   GObject    *item)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);
  GSList *pending;

  pending = g_hash_table_lookup (priv->pending_messages, channel);
  pending = g_slist_prepend (pending, GUINT_TO_POINTER (id));
  g_hash_table_insert (priv->pending_messages, channel, pending);

  anerley_item_emit_unread_messages_changed (ANERLEY_ITEM (item),
      anerley_tp_item_get_unread_messages_count (ANERLEY_ITEM (item)));
}

static void
_get_pending_messages (TpChannel       *channel,
                       const GPtrArray *messages,
                       const GError    *in_error,
                       gpointer         user_data,
                       GObject         *item)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);
  GSList *pending;
  int i;

  pending = g_hash_table_lookup (priv->pending_messages, channel);

  if (in_error != NULL)
    {
      g_critical ("Error retrieving pending messages: %s", in_error->message);
      return;
    }

  for (i = 0; i < messages->len; i++)
    {
      GValueArray *message = g_ptr_array_index (messages, i);
      guint id = g_value_get_uint (g_value_array_get_nth (message, 0));

      pending = g_slist_prepend (pending, GUINT_TO_POINTER (id));
    }

  g_hash_table_insert (priv->pending_messages, channel, pending);

  anerley_item_emit_unread_messages_changed (ANERLEY_ITEM (item),
      anerley_tp_item_get_unread_messages_count (ANERLEY_ITEM (item)));
}

static void
_message_acknowledged (TpChannel    *channel,
                       const GArray *ids,
                       gpointer      user_data,
                       GObject      *item)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);
  GSList *pending;
  int i;

  pending = g_hash_table_lookup (priv->pending_messages, channel);

  for (i = 0; i < ids->len; i++)
    {
      guint id = g_array_index (ids, guint, i);

      pending = g_slist_remove (pending, GUINT_TO_POINTER (id));
    }

  g_hash_table_insert (priv->pending_messages, channel, pending);

  anerley_item_emit_unread_messages_changed (ANERLEY_ITEM (item),
      anerley_tp_item_get_unread_messages_count (ANERLEY_ITEM (item)));
}

static void
_channel_closed (TpChannel *channel,
                 gpointer   user_data,
                 GObject   *item)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);
  GSList *pending;

  pending = g_hash_table_lookup (priv->pending_messages, channel);
  g_slist_free (pending);
  g_hash_table_remove (priv->pending_messages, channel);
  g_object_unref (channel);

  anerley_item_emit_unread_messages_changed (ANERLEY_ITEM (item),
      anerley_tp_item_get_unread_messages_count (ANERLEY_ITEM (item)));
}

/**
 * anerley_tp_item_associate_channel:
 * @item: an #AnerleyTpItem
 * @channel: a prepared #TpChannel
 *
 * Associates this channel with the #AnerleyTpItem to track the unread
 * messages in the channel.
 */
void
anerley_tp_item_associate_channel (AnerleyTpItem *item,
                                   TpChannel     *channel)
{
  AnerleyTpItemPrivate *priv = GET_PRIVATE (item);
  GError *error = NULL;

  g_return_if_fail (ANERLEY_IS_TP_ITEM (item));

  /* We only want to do message counts if we have the Messages interface */

  if (!tp_proxy_has_interface_by_id (channel,
                                     TP_IFACE_QUARK_CHANNEL_INTERFACE_MESSAGES))
    return;

  g_hash_table_insert (priv->pending_messages, g_object_ref (channel), NULL);

  tp_cli_channel_type_text_connect_to_received (channel, _message_received,
      NULL, NULL, G_OBJECT (item), &error);
  g_assert_no_error (error);

  tp_cli_channel_interface_messages_connect_to_pending_messages_removed (
      channel, _message_acknowledged,
      NULL, NULL, G_OBJECT (item), &error);
  g_assert_no_error (error);

  tp_cli_channel_connect_to_closed (channel, _channel_closed,
      NULL, NULL, G_OBJECT (item), &error);
  g_assert_no_error (error);

  tp_cli_channel_type_text_call_list_pending_messages (channel, -1, FALSE,
      _get_pending_messages,
      NULL, NULL, G_OBJECT (item));
}
