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
#include <folks/folks-telepathy.h>

#include "anerley-item.h"

G_DEFINE_TYPE (AnerleyItem, anerley_item, G_TYPE_OBJECT)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_ITEM, AnerleyItemPrivate))

#define GET_PRIVATE(o) \
    ((AnerleyItem *)o)->priv

struct _AnerleyItemPrivate {
  FolksIndividual *contact;

  gchar *display_name;
  GLoadableIcon *avatar;
  gchar *presence_message;
  gchar *sortable_name;

  const gchar *presence_status;

  GHashTable *pending_messages; /* reffed TpChannel* to GSList of guint */
};

enum
{
  PROP_0,
  PROP_CONTACT,
};

enum
{
  DISPLAY_NAME_CHANGED,
  AVATAR_CHANGED,
  PRESENCE_CHANGED,
  UNREAD_MESSAGES_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void anerley_item_update_contact (AnerleyItem *item,
                                         FolksIndividual     *contact);

static void
anerley_item_get_property (GObject *object, guint property_id,
                           GValue *value, GParamSpec *pspec)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_CONTACT:
      g_value_set_object (value, priv->contact);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_item_set_property (GObject *object, guint property_id,
                           const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
    case PROP_CONTACT:
      anerley_item_update_contact ((AnerleyItem *)object, 
                                      (FolksIndividual *)g_value_get_object (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_item_dispose (GObject *object)
{
  AnerleyItem *item = (AnerleyItem *)object;
  AnerleyItemPrivate *priv = GET_PRIVATE (object);

  anerley_item_update_contact (item, NULL);

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

  G_OBJECT_CLASS (anerley_item_parent_class)->dispose (object);
}

static void
anerley_item_finalize (GObject *object)
{
  G_OBJECT_CLASS (anerley_item_parent_class)->finalize (object);
}

const gchar *
anerley_item_get_display_name (AnerleyItem *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);

  return priv->display_name;
}

const gchar *
anerley_item_get_sortable_name (AnerleyItem *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);

  return priv->sortable_name;
}

GLoadableIcon *
anerley_item_get_avatar (AnerleyItem *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);

  return priv->avatar;
}

const gchar *
anerley_item_get_presence_status (AnerleyItem *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);

  return priv->presence_status;
}

const gchar *
anerley_item_get_presence_message (AnerleyItem *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);

  return priv->presence_message;
}

FolksPresenceType
anerley_item_get_presence_type (AnerleyItem *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);

  return folks_presence_details_get_presence_type (
      (FolksPresenceDetails *) priv->contact);
}

gboolean
anerley_item_is_im (AnerleyItem *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);
  FolksPresenceType presence;

  presence = folks_presence_details_get_presence_type (
      (FolksPresenceDetails *) priv->contact);

  return presence != FOLKS_PRESENCE_TYPE_UNSET;
}

gboolean
anerley_item_is_online (AnerleyItem *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);
  FolksPresenceType presence;

  presence = folks_presence_details_get_presence_type (
      (FolksPresenceDetails *) priv->contact);

  return presence > FOLKS_PRESENCE_TYPE_OFFLINE &&
         presence < FOLKS_PRESENCE_TYPE_UNKNOWN;
}

guint
anerley_item_get_unread_messages_count (AnerleyItem *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);
  GHashTableIter iter;
  gpointer key, value;
  guint total = 0;

  g_return_val_if_fail (ANERLEY_IS_ITEM (item), 0);

  g_hash_table_iter_init (&iter, priv->pending_messages);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      total += GPOINTER_TO_UINT (value);
    }

  return total;
}

void
anerley_item_emit_display_name_changed (AnerleyItem *item)
{
  g_signal_emit (item, signals[DISPLAY_NAME_CHANGED], 0);
}

void
anerley_item_emit_avatar_changed (AnerleyItem *item)
{
  g_signal_emit (item, signals[AVATAR_CHANGED], 0);
}

void
anerley_item_emit_presence_changed (AnerleyItem *item)
{
  g_signal_emit (item, signals[PRESENCE_CHANGED], 0);
}

void
anerley_item_emit_unread_messages_changed (AnerleyItem *item)
{
  g_signal_emit (item, signals[UNREAD_MESSAGES_CHANGED], 0,
                 anerley_item_get_unread_messages_count (item));
}

void
anerley_item_activate (AnerleyItem *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);
  GeeSet *personas;
  GeeIterator *iter;
  GList *contacts = NULL;
  TpContact *contact;
  TpConnection *connection;
  TpAccount *account;
  TpAccountChannelRequest *acr;
  GHashTable *request;

  /* Collect all merged TpContacts */
  personas = folks_individual_get_personas (priv->contact);
  iter = gee_iterable_iterator (GEE_ITERABLE (personas));
  while (gee_iterator_next (iter))
  {
    FolksPersona *persona = gee_iterator_get (iter);

    if (TPF_IS_PERSONA (persona))
    {
      TpContact *tp_contact;

      tp_contact = tpf_persona_get_contact (TPF_PERSONA (persona));
      contacts = g_list_prepend (contacts, g_object_ref (tp_contact));
    }

    g_clear_object (&persona);
  }
  g_clear_object (&iter);

  if (contacts == NULL)
    return;

  /* FIXME: Need to select which contact, pick first one for now... */
  contact = contacts->data;
  connection = tp_contact_get_connection (contact);
  account = tp_connection_get_account (connection);

  /* FIXME: Need to select which action, chat for now... */
  request = tp_asv_new (
      TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
      TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, TP_HANDLE_TYPE_CONTACT,
      TP_PROP_CHANNEL_TARGET_HANDLE, G_TYPE_UINT, tp_contact_get_handle (contact),
      NULL);

  acr = tp_account_channel_request_new (account, request,
                                        TP_USER_ACTION_TIME_CURRENT_TIME);
  tp_account_channel_request_ensure_channel_async (acr, NULL, NULL, NULL, NULL);

  g_hash_table_unref (request);
  g_object_unref (acr);
  g_list_free_full (contacts, g_object_unref);
}

void
anerley_item_close (AnerleyItem *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);
  GHashTableIter iter;
  gpointer key;

  g_hash_table_iter_init (&iter, priv->pending_messages);
  while (g_hash_table_iter_next (&iter, &key, NULL))
    {
      tp_channel_destroy_async (key, NULL, NULL);
    }
}

static void
anerley_item_class_init (AnerleyItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyItemPrivate));

  object_class->get_property = anerley_item_get_property;
  object_class->set_property = anerley_item_set_property;
  object_class->dispose = anerley_item_dispose;
  object_class->finalize = anerley_item_finalize;

  pspec = g_param_spec_object ("contact",
                               "The contact",
                               "Underlying contact whose details we represent",
                               FOLKS_TYPE_INDIVIDUAL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_CONTACT, pspec);

  signals[DISPLAY_NAME_CHANGED] =
    g_signal_new ("display-name-changed",
                  ANERLEY_TYPE_ITEM,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  signals[AVATAR_CHANGED] =
    g_signal_new ("avatar-changed",
                  ANERLEY_TYPE_ITEM,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
  signals[PRESENCE_CHANGED] =
    g_signal_new ("presence_changed",
                  ANERLEY_TYPE_ITEM,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
  signals[UNREAD_MESSAGES_CHANGED] =
    g_signal_new ("unread-messages-changed",
                  ANERLEY_TYPE_ITEM,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__UINT,
                  G_TYPE_NONE,
                  1, G_TYPE_UINT);
}

static void
anerley_item_init (AnerleyItem *self)
{
  AnerleyItemPrivate *priv = GET_PRIVATE_REAL (self);
  self->priv = priv;

  priv->pending_messages = g_hash_table_new_full (NULL, NULL,
                                                  g_object_unref, NULL);
}

AnerleyItem *
anerley_item_new (FolksIndividual *contact)
{
  return g_object_new (ANERLEY_TYPE_ITEM,
                       "contact", contact,
                       NULL);
}

static void
_contact_notify_alias_cb (GObject    *object,
                          GParamSpec *pspec,
                          gpointer    userdata)
{
  FolksIndividual *contact = (FolksIndividual *)object;
  AnerleyItem *item = (AnerleyItem *)userdata;
  AnerleyItemPrivate *priv = GET_PRIVATE (item);
  const gchar *alias;

  g_free (priv->display_name);
  priv->display_name = NULL;
  g_free (priv->sortable_name);
  priv->sortable_name = NULL;

  alias = folks_alias_details_get_alias (FOLKS_ALIAS_DETAILS (contact));

  if (alias)
  {
    priv->display_name = g_strdup (alias);
    priv->sortable_name = g_utf8_casefold (alias, -1);
  }

  anerley_item_emit_display_name_changed ((AnerleyItem *)item);
}

static const gchar *
_get_real_presence_status (FolksIndividual *contact)
{
  FolksPresenceType presence;

  presence = folks_presence_details_get_presence_type (
      (FolksPresenceDetails *) contact);

  switch (presence)
  {
    case FOLKS_PRESENCE_TYPE_UNSET:
      return "offline";
    case FOLKS_PRESENCE_TYPE_OFFLINE:
      return "offline";
    case FOLKS_PRESENCE_TYPE_AVAILABLE:
      return "available";
    case FOLKS_PRESENCE_TYPE_AWAY:
      return "away";
    case FOLKS_PRESENCE_TYPE_EXTENDED_AWAY:
      return "xa";
    case FOLKS_PRESENCE_TYPE_HIDDEN:
      return "offline";
    case FOLKS_PRESENCE_TYPE_BUSY:
      return "dnd";
    case FOLKS_PRESENCE_TYPE_UNKNOWN:
      return "offline";
    case FOLKS_PRESENCE_TYPE_ERROR:
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
  FolksIndividual *contact = (FolksIndividual *)object;
  AnerleyItem *item = (AnerleyItem *)userdata;
  AnerleyItemPrivate *priv = GET_PRIVATE (item);

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
  FolksIndividual *contact = (FolksIndividual *)object;
  AnerleyItem *item = (AnerleyItem *)userdata;
  AnerleyItemPrivate *priv = GET_PRIVATE (item);

  g_free (priv->presence_message);
  priv->presence_message = NULL;

  priv->presence_message = g_strdup (folks_presence_details_get_presence_message (
      (FolksPresenceDetails *) contact));

  anerley_item_emit_presence_changed ((AnerleyItem *)item);
}

static void
_contact_notify_avatar_cb (GObject    *object,
                           GParamSpec *pspec,
                           gpointer    userdata)
{
  FolksIndividual *contact = (FolksIndividual *)object;
  AnerleyItem *item = (AnerleyItem *)userdata;
  AnerleyItemPrivate *priv = GET_PRIVATE (item);
  GLoadableIcon *avatar;

  g_clear_object (&priv->avatar);
  avatar = folks_avatar_details_get_avatar (FOLKS_AVATAR_DETAILS (contact));

  if (avatar)
    priv->avatar = g_object_ref (avatar);

  anerley_item_emit_avatar_changed ((AnerleyItem *)item);
}

static void
anerley_item_update_contact (AnerleyItem *item,
                             FolksIndividual     *contact)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);

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

  g_clear_object (&priv->avatar);

  g_free (priv->display_name);
  g_free (priv->presence_message);

  priv->display_name = NULL;
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
    g_signal_connect (priv->contact,
                      "notify::avatar",
                      (GCallback)_contact_notify_avatar_cb,
                      item);

    /* Simulate the hook-ups */
    g_object_notify ((GObject *)priv->contact,
                     "alias");
    g_object_notify ((GObject *)priv->contact,
                     "presence-status");
    g_object_notify ((GObject *)priv->contact,
                     "presence-message");
    g_object_notify ((GObject *)priv->contact,
                     "avatar");
  }
}

static void
_message_received_cb (TpTextChannel      *channel,
                      TpSignalledMessage *message,
                      AnerleyItem      *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);
  guint count;

  count = GPOINTER_TO_UINT (g_hash_table_lookup (priv->pending_messages,
                                                 channel));
  g_hash_table_insert (priv->pending_messages,
                       g_object_ref (channel),
                       GUINT_TO_POINTER (++count));

  anerley_item_emit_unread_messages_changed (ANERLEY_ITEM (item));
}

static void
_message_acknowledged_cb (TpTextChannel      *channel,
                          TpSignalledMessage *message,
                          AnerleyItem      *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);
  guint count;

  count = GPOINTER_TO_UINT (g_hash_table_lookup (priv->pending_messages,
                                                 channel));
  g_hash_table_insert (priv->pending_messages,
                       g_object_ref (channel),
                       GUINT_TO_POINTER (--count));

  anerley_item_emit_unread_messages_changed (ANERLEY_ITEM (item));
}

static void
_channel_invalidated_cb (TpChannel     *channel,
                         guint          domain,
                         gint           code,
                         gchar         *message,
                         AnerleyItem *item)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);

  g_hash_table_remove (priv->pending_messages, channel);

  anerley_item_emit_unread_messages_changed (ANERLEY_ITEM (item));
}

/**
 * anerley_item_associate_channel:
 * @item: an #AnerleyItem
 * @channel: a prepared #TpTextChannel
 *
 * Associates this channel with the #AnerleyItem to track the unread
 * messages in the channel.
 */
void
anerley_item_associate_channel (AnerleyItem *item,
                                TpTextChannel *channel)
{
  AnerleyItemPrivate *priv = GET_PRIVATE (item);
  GList *pending;

  g_return_if_fail (ANERLEY_IS_ITEM (item));
  g_return_if_fail (TP_IS_TEXT_CHANNEL (channel));
  g_return_if_fail (g_hash_table_lookup (priv->pending_messages, channel) == NULL);

  tp_g_signal_connect_object (channel, "message-received",
                              G_CALLBACK (_message_received_cb),
                              item, 0);
  tp_g_signal_connect_object (channel, "pending-message-removed",
                              G_CALLBACK (_message_acknowledged_cb),
                              item, 0);
  tp_g_signal_connect_object (channel, "invalidated",
                              G_CALLBACK (_channel_invalidated_cb),
                              item, 0);

  pending = tp_text_channel_get_pending_messages (TP_TEXT_CHANNEL (channel));
  g_hash_table_insert (priv->pending_messages,
                       g_object_ref (channel),
                       GUINT_TO_POINTER (g_list_length (pending)));

  anerley_item_emit_unread_messages_changed (ANERLEY_ITEM (item));

  g_list_free (pending);
}
