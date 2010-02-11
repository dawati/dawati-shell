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

#include <telepathy-glib/account.h>
#include <telepathy-glib/contact.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/channel.h>
#include <telepathy-glib/interfaces.h>

#include "anerley-feed.h"
#include "anerley-item.h"
#include "anerley-tp-feed.h"
#include "anerley-tp-item.h"

static void feed_interface_init (gpointer g_iface, gpointer iface_data);
G_DEFINE_TYPE_WITH_CODE (AnerleyTpFeed,
                         anerley_tp_feed,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (ANERLEY_TYPE_FEED,
                                                feed_interface_init));

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_TP_FEED, AnerleyTpFeedPrivate))

typedef struct _AnerleyTpFeedPrivate AnerleyTpFeedPrivate;

struct _AnerleyTpFeedPrivate {
  TpAccount *account;
  TpConnection *conn;
  TpChannel *subscribe_channel;

  GHashTable *ids_to_items;
  GHashTable *handles_to_ids;
  GHashTable *handles_to_contacts;
};

enum
{
  PROP_0,
  PROP_ACCOUNT,
  PROP_ONLINE
};

static void
anerley_tp_feed_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AnerleyTpFeed *feed = ANERLEY_TP_FEED (object);
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_ACCOUNT:
      g_value_set_object (value, priv->account);
      break;
    case PROP_ONLINE:
      g_value_set_boolean (value, anerley_tp_feed_get_online (feed));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tp_feed_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_ACCOUNT:
      priv->account = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void _account_status_changed_cb (TpAccount                *account,
                                        TpConnectionStatus        old_status,
                                        TpConnectionStatus        new_status,
                                        TpConnectionStatusReason  reason,
                                        const gchar              *dbus_error_name,
                                        const GHashTable         *details,
                                        gpointer                  userdata);

static void
anerley_tp_feed_dispose (GObject *object)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (object);

  /*
   * Do here rather than finalize since this will then cause the TpContact
   * inside the item to be unreffed.
   */
  if (priv->handles_to_contacts)
  {
    g_hash_table_unref (priv->handles_to_contacts);
    priv->handles_to_contacts = NULL;
  }

  if (priv->ids_to_items)
  {
    g_hash_table_unref (priv->ids_to_items);
    priv->ids_to_items = NULL;
  }

  if (priv->handles_to_ids)
  {
    g_hash_table_unref (priv->handles_to_ids);
    priv->handles_to_ids = NULL;
  }

  if (priv->subscribe_channel)
  {
    g_object_unref (priv->subscribe_channel);
    priv->subscribe_channel = NULL;
  }

  if (priv->conn)
  {
    g_object_unref (priv->conn);
    priv->conn = NULL;
  }

  if (priv->account)
  {
    g_signal_handlers_disconnect_by_func (priv->account,
                                          _account_status_changed_cb,
                                          object);

    g_object_unref (priv->account);
    priv->account = NULL;
  }

  G_OBJECT_CLASS (anerley_tp_feed_parent_class)->dispose (object);
}

static void
anerley_tp_feed_finalize (GObject *object)
{
  G_OBJECT_CLASS (anerley_tp_feed_parent_class)->finalize (object);
}

static void
_tp_connection_request_avatars_cb (TpConnection *conn,
                                   const GError *error,
                                   gpointer      userdata,
                                   GObject      *weak_object)
{
  if (error)
  {
    g_warning (G_STRLOC ": RequestAvatars call failed: %s",
               error->message);
  }
}

static AnerleyTpItem *
_make_item_from_contact (AnerleyTpFeed *feed,
                         TpContact     *contact)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  AnerleyTpItem *item;

  item = anerley_tp_item_new (priv->account,
                              contact);


  /* Move the ownership of the reference to the hash table */
  g_hash_table_insert (priv->ids_to_items,
                       g_strdup (tp_contact_get_identifier (contact)),
                       item);

  /*
   * We need to save the handle -> id mapping so that when we deal with a
   * removal, well, so that we can :-)
   */
  g_hash_table_insert (priv->handles_to_ids,
                       GUINT_TO_POINTER (tp_contact_get_handle (contact)),
                       g_strdup (tp_contact_get_identifier (contact)));

  return item;
}

/* return TRUE if avatar found and set, otherwise return FALSE */
static gboolean
_set_avatar_if_present (AnerleyTpFeed *feed,
                        AnerleyTpItem *item,
                        TpContact     *contact)
{
  const gchar *token;
  gchar *avatar_path = NULL;
  gboolean res = FALSE;

  /* Only try and find an avatar file if we have an avatar token */
  token = tp_contact_get_avatar_token (contact);

  if (token && !g_str_equal (token, ""))
  {
    /* Check if contact's avatar is already downloaded */
    avatar_path = g_build_filename (g_get_user_cache_dir (),
                                    "anerley",
                                    "avatars",
                                    token,
                                    NULL);
    if (g_file_test (avatar_path, G_FILE_TEST_EXISTS))
    {
      /* Already downloaded, woot! */
      anerley_tp_item_set_avatar_path (item, avatar_path);
      res = TRUE;
    } else {
      res = FALSE;
    }

    g_free (avatar_path);
  }

  return res;
}

static void
_tp_contact_presence_type_changed (GObject    *object,
                                   GParamSpec *pspec,
                                   gpointer    userdata)
{
  AnerleyTpFeed *feed = ANERLEY_TP_FEED (userdata);
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (userdata);
  TpContact *contact = (TpContact *)object;
  TpConnectionPresenceType presence_type;
  AnerleyTpItem *item;
  GList *items = NULL;
  GArray *missing_avatar_handles;
  TpHandle handle;

  missing_avatar_handles = g_array_new (TRUE, TRUE, sizeof (TpHandle));

  presence_type = tp_contact_get_presence_type (contact);

  if (presence_type == TP_CONNECTION_PRESENCE_TYPE_OFFLINE)
  {
    item = g_hash_table_lookup (priv->ids_to_items,
                                tp_contact_get_identifier (contact));

    if (item)
    {
      items = g_list_append (items, item);
      g_signal_emit_by_name (feed, "items-removed", items);
      g_hash_table_remove (priv->ids_to_items,
                           tp_contact_get_identifier (contact));
      g_list_free (items);
    }
  } else {
    item = g_hash_table_lookup (priv->ids_to_items,
                                tp_contact_get_identifier (contact));

    if (!item)
    {
      item = _make_item_from_contact (feed, contact);

      if (!_set_avatar_if_present (feed, item, contact))
      {
         handle = tp_contact_get_handle (contact);
         g_array_append_val (missing_avatar_handles,
                             handle);
      }

      items = g_list_append (items, item);
      g_signal_emit_by_name (feed, "items-added", items);
      g_list_free (items);
    }
  }

  if (missing_avatar_handles->len > 0)
  {
    tp_cli_connection_interface_avatars_call_request_avatars (priv->conn,
                                                              -1,
                                                              missing_avatar_handles,
                                                              _tp_connection_request_avatars_cb,
                                                              NULL,
                                                              NULL,
                                                              (GObject *)feed);
    g_array_free (missing_avatar_handles, TRUE);
  }
}

static void
_tp_connection_get_contacts_cb (TpConnection      *connection,
                                guint              n_contacts,
                                TpContact * const *contacts,
                                guint              n_failed,
                                const TpHandle    *failed,
                                const GError      *error,
                                gpointer           userdata,
                                GObject           *weak_object)
{
  AnerleyTpFeed *feed = ANERLEY_TP_FEED (weak_object);
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  gint i = 0;
  TpContact *contact;
  AnerleyTpItem *item;
  GList *changed_items = NULL;
  GList *added_items = NULL;
  GArray *missing_avatar_handles;
  TpHandle handle;

  if (error)
  {
    g_warning (G_STRLOC ": Error when fetching contacts: %s",
               error->message);
    return;
  }

  missing_avatar_handles = g_array_new (TRUE, TRUE, sizeof (TpHandle));

  for (i = 0; i < n_contacts; i++)
  {
    contact = (TpContact *)contacts[i];
#if 0
    g_debug (G_STRLOC ": Got contact: %s (%s) - %s",
             tp_contact_get_alias (contact),
             tp_contact_get_identifier (contact),
             tp_contact_get_presence_status (contact));
#endif

    g_hash_table_insert (priv->handles_to_contacts,
                         GUINT_TO_POINTER (tp_contact_get_handle (contact)),
                         g_object_ref (contact));

    g_signal_connect (contact,
                      "notify::presence-type",
                      (GCallback)_tp_contact_presence_type_changed,
                      feed);


    if (tp_contact_get_presence_type (contact) == TP_CONNECTION_PRESENCE_TYPE_OFFLINE)
    {
      /* Skip offline contacts */
      continue;
    }

    item = g_hash_table_lookup (priv->ids_to_items,
                                tp_contact_get_identifier (contact));

    if (item)
    {
      g_object_set (item,
                    "contact",
                    contact,
                    NULL);
      changed_items = g_list_append (changed_items, item);
    } else {
      item = _make_item_from_contact (feed, contact);
      added_items = g_list_append (added_items, item);
    }

    if (!_set_avatar_if_present (feed, item, contact))
    {
       handle = tp_contact_get_handle (contact);
       g_array_append_val (missing_avatar_handles,
                          handle);
    }
  }

  /* TODO: Move to idle to avoid blocking the bus */

  /* We are holding an implicit reference here because the hash table has it.
   * The expectation is that the UI components take the reference on the item
   * if they want it.
   */
  if (added_items)
    g_signal_emit_by_name (feed,
                           "items-added",
                           added_items);

  if (changed_items)
    g_signal_emit_by_name (feed,
                           "items-changed",
                           changed_items);

  g_list_free (added_items);
  g_list_free (changed_items);

  if (missing_avatar_handles->len > 0)
  {
    tp_cli_connection_interface_avatars_call_request_avatars (priv->conn,
                                                              -1,
                                                              missing_avatar_handles,
                                                              _tp_connection_request_avatars_cb,
                                                              NULL,
                                                              NULL,
                                                              (GObject *)feed);
    g_array_free (missing_avatar_handles, TRUE);
  }
}



static void
anerley_tp_feed_fetch_contacts (AnerleyTpFeed *feed,
                                const GArray  *members)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  TpContactFeature features[] = {TP_CONTACT_FEATURE_ALIAS,
                                 TP_CONTACT_FEATURE_AVATAR_TOKEN,
                                 TP_CONTACT_FEATURE_PRESENCE };

  /* Now we have our members let's request the TpContacts for them */
  tp_connection_get_contacts_by_handle (priv->conn,
                                        members->len,
                                        (TpHandle *)members->data,
                                        3,
                                        features,
                                        _tp_connection_get_contacts_cb,
                                        NULL,
                                        NULL,
                                        (GObject *)feed);
}

static void
_tp_channel_members_changed_on_subscribe_cb (TpChannel    *proxy,
                                             const gchar  *message,
                                             const GArray *added,
                                             const GArray *removed,
                                             const GArray *local_pending,
                                             const GArray *remote_pending,
                                             guint         actor,
                                             guint         reason,
                                             gpointer      userdata,
                                             GObject      *weak_object)
{
  AnerleyTpFeed *feed = ANERLEY_TP_FEED (weak_object);
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  TpHandle handle;
  gchar *id;
  AnerleyItem *item;
  gint i = 0;
  GList *removed_items = NULL;
  GList *l;
  TpContact *contact;

  if (added->len > 0)
    anerley_tp_feed_fetch_contacts (feed, added);

  for (i = 0; i < removed->len; i++)
  {
    handle = (TpHandle)removed->data[i];
    id = g_hash_table_lookup (priv->handles_to_ids,
                              GUINT_TO_POINTER (handle));

    if (id)
    {
      item = g_hash_table_lookup (priv->ids_to_items, id);

      if (item)
      {
        removed_items = g_list_append (removed_items, item);
      }

      g_hash_table_remove (priv->handles_to_ids,
                           GUINT_TO_POINTER (handle));
    }

    g_hash_table_remove (priv->handles_to_contacts,
                         GUINT_TO_POINTER (handle));
  }

  if (removed_items)
    g_signal_emit_by_name (feed,
                           "items-removed",
                           removed_items);

  for (l = removed_items; l; l = l->next)
  {
    item = (AnerleyItem *)l->data;
    g_object_get (item,
                  "contact",
                  &contact,
                  NULL);
    g_hash_table_remove (priv->ids_to_items,
                         tp_contact_get_identifier (contact));
  }
}

static void
_tp_channel_get_all_members_for_subscribe_cb (TpChannel    *proxy,
                                              const GArray *members,
                                              const GArray *local_pending,
                                              const GArray *remote_pending,
                                              const GError *error,
                                              gpointer      userdata,
                                              GObject      *weak_object)
{
  AnerleyTpFeed *feed = ANERLEY_TP_FEED (weak_object);

  if (error)
  {
    g_warning (G_STRLOC ": Error whilst getting all members on the channel: %s",
               error->message);
    return;
  }

  if (members->len > 0)
    anerley_tp_feed_fetch_contacts (feed, members);
}

static void
_tp_channel_ready_for_subscribe_cb (TpChannel    *channel,
                                    const GError *error_in,
                                    gpointer      userdata)
{
  AnerleyTpFeed *feed = ANERLEY_TP_FEED (userdata);
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  GError *error = NULL;

  if (error_in)
  {
    g_warning (G_STRLOC ": Error whilst waiting for our channel to become ready: %s",
               error_in->message);
    return;
  }

  /* So finally we can now have a working channel. Lets hook onto the
   * MembersChanged signal so we can be told when the membership changes.
   */

  tp_cli_channel_interface_group_connect_to_members_changed (priv->subscribe_channel,
                                                             _tp_channel_members_changed_on_subscribe_cb,
                                                             NULL,
                                                             NULL,
                                                             (GObject *)feed,
                                                             &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error whilst connecting to members changed signal: %s",
               error->message);
    g_clear_error (&error);
  }

  /* Now lets ask for the members of this channel */
  tp_cli_channel_interface_group_call_get_all_members (priv->subscribe_channel,
                                                       -1,
                                                       _tp_channel_get_all_members_for_subscribe_cb,
                                                       NULL,
                                                       NULL,
                                                       (GObject *)feed);

}

static void
_tp_connection_request_channel_for_subscribe_cb (TpConnection *proxy,
                                                 const gchar  *object_path,
                                                 const GError  *error_in,
                                                 gpointer      userdata,
                                                 GObject      *weak_object)
{
  AnerleyTpFeed *feed = ANERLEY_TP_FEED (weak_object);
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  TpHandle handle = GPOINTER_TO_UINT (userdata);
  GError *error = NULL;

  if (error_in)
  {
    g_warning (G_STRLOC ": Error requesting subscribe channel: %s",
               error_in->message);
    return;
  }

  priv->subscribe_channel = tp_channel_new (priv->conn,
                                            object_path,
                                            TP_IFACE_CHANNEL_TYPE_CONTACT_LIST,
                                            TP_HANDLE_TYPE_LIST,
                                            handle,
                                            &error);

  if (!priv->subscribe_channel)
  {
    g_warning (G_STRLOC ": Error setting up channel: %s",
               error->message);
    return;
  }

  tp_channel_call_when_ready (priv->subscribe_channel,
                              _tp_channel_ready_for_subscribe_cb,
                              feed);
}

static void
_tp_connection_request_handle_for_subscribe_cb (TpConnection        *connection,
                                                TpHandleType         handle_type,
                                                guint                n_handles,
                                                const TpHandle      *handles,
                                                const gchar * const *requested_ids,
                                                const GError        *error,
                                                gpointer             userdata,
                                                GObject             *weak_object)
{
  AnerleyTpFeed *feed = ANERLEY_TP_FEED (weak_object);
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);

  if (error)
  {
    g_warning (G_STRLOC ": Error requesting handle for subscribe channel: %s",
               error->message);
    return;
  }

  /* Okay now we have the handle then it's party time. Uh...time to actually
   * request the channel.
   */
  tp_cli_connection_call_request_channel (priv->conn,
                                          -1,
                                          TP_IFACE_CHANNEL_TYPE_CONTACT_LIST,
                                          TP_HANDLE_TYPE_LIST,
                                          handles[0],
                                          TRUE,
                                          _tp_connection_request_channel_for_subscribe_cb,
                                          GUINT_TO_POINTER (handles[0]),
                                          NULL,
                                          (GObject *)feed);
}

static void
anerley_tp_feed_setup_subscribe_channel (AnerleyTpFeed *feed)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  const gchar *channel_names[] = { "subscribe", NULL };

  /* Since all we care about right now is the subcribe contact list. Let's
   * just request that channel directly
   */

  /* First we have to look up the handle for the list we want. This is a bit
   * lame since it involves a round-trip, oh well.
   */

  tp_connection_request_handles (priv->conn,
                                 -1,           /* use default */
                                 TP_HANDLE_TYPE_LIST,
                                 channel_names, 
                                 _tp_connection_request_handle_for_subscribe_cb,
                                 NULL,
                                 NULL,
                                 (GObject *)feed); /* I love weak objects */
}

static void
_tp_connection_avatar_retrieved_cb (TpConnection *conn,
                                    TpHandle      handle,
                                    const gchar  *token,
                                    const GArray *image_data,
                                    const char   *type,
                                    gpointer      userdata,
                                    GObject      *weak_object)
{
  AnerleyTpFeed *feed = (AnerleyTpFeed *)weak_object;
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  gchar *avatar_path;
  GError *error = NULL;
  const gchar *id;
  AnerleyTpItem *item;

  avatar_path = g_build_filename (g_get_user_cache_dir (),
                                  "anerley",
                                  "avatars",
                                  token,
                                  NULL);

  if (!g_file_set_contents (avatar_path,
                            (gchar *)image_data->data,
                            image_data->len,
                            &error))
  {
    g_warning (G_STRLOC ": Error creating avatar file: %s",
               error->message);
    g_clear_error (&error);
  }

  id = g_hash_table_lookup (priv->handles_to_ids,
                            GUINT_TO_POINTER (handle));

  if (id)
  {
    item = g_hash_table_lookup (priv->ids_to_items,
                                id);

    if (item)
    {
      anerley_tp_item_set_avatar_path (item, avatar_path);
    }
  }

  g_free (avatar_path);
}

static void
_tp_connection_ready_cb (TpConnection *connection,
                         const GError *error_in,
                         gpointer      userdata)
{
  AnerleyTpFeed *feed = (AnerleyTpFeed *)userdata;
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  GError *error = NULL;

  if (error_in)
  {
    g_warning (G_STRLOC ": Error. Connection is now invalid: %s",
               error_in->message);
    return;
  }

  /* The next thing we care about is getting the list of contacts we subscribe
   * to. That requires us to request and set up an appropriate channel.
   */

  anerley_tp_feed_setup_subscribe_channel (feed);

  /* Connect to the avatar retrieved signal */
  tp_cli_connection_interface_avatars_connect_to_avatar_retrieved (priv->conn,
                                                                   _tp_connection_avatar_retrieved_cb,
                                                                   NULL,
                                                                   NULL,
                                                                   (GObject *)feed,
                                                                   &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error connecting to AvatarRetrieved signal: %s",
               error->message);
    g_clear_error (&error);
  }
}

static void
_connection_notify_status_cb (TpConnection *connection,
                              GParamSpec   *pspec,
                              gpointer      userdata)
{
  AnerleyTpFeed *feed = ANERLEY_TP_FEED (userdata);
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  GList *items;
  TpConnectionStatus status;

  status = tp_connection_get_status (priv->conn, NULL);

  g_debug (G_STRLOC ": Connection is in state: %s",
           status==TP_CONNECTION_STATUS_CONNECTED ? "connected" :
           status==TP_CONNECTION_STATUS_CONNECTING ? "connecting" :
           status==TP_CONNECTION_STATUS_DISCONNECTED ? "disconnected" :
           "other");

  if (status == TP_CONNECTION_STATUS_DISCONNECTED)
  {
    /*
     * This means our connection has been disconnected. That is :-( so lets
     * remove these things from our internal store and emit the signals on
     * the feed.
     */

    items = g_hash_table_get_values (priv->ids_to_items);

    g_signal_emit_by_name (feed, "items-removed", items);
    g_list_free (items);
    g_hash_table_remove_all (priv->handles_to_contacts);
    g_hash_table_remove_all (priv->ids_to_items);
    g_hash_table_remove_all (priv->handles_to_ids);

    if (priv->subscribe_channel)
    {
      g_object_unref (priv->subscribe_channel);
      priv->subscribe_channel = NULL;
    }

    if (priv->conn)
    {
      g_object_unref (priv->conn);
      priv->conn = NULL;
    }

    g_object_notify ((GObject *)feed, "online");
  }
}

static void
_account_status_changed_cb (TpAccount                *account,
                            TpConnectionStatus        old_status,
                            TpConnectionStatus        status,
                            TpConnectionStatusReason  reason,
                            const gchar              *dbus_error_name,
                            const GHashTable         *details,
                            gpointer                  userdata)
{
  AnerleyTpFeed *feed = ANERLEY_TP_FEED (userdata);
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);

  g_debug (G_STRLOC ": Connection is in state: %s",
           status==TP_CONNECTION_STATUS_CONNECTED ? "connected" :
           status==TP_CONNECTION_STATUS_CONNECTING ? "connecting" :
           status==TP_CONNECTION_STATUS_DISCONNECTED ? "disconnected" :
           "other");

  if (status == TP_CONNECTION_STATUS_CONNECTED)
  {
    priv->conn = g_object_ref (tp_account_get_connection (account));

    g_signal_connect (priv->conn,
                      "notify::status",
                      (GCallback)_connection_notify_status_cb,
                      feed);

    tp_connection_call_when_ready (priv->conn,
                                   _tp_connection_ready_cb,
                                   feed);
    g_object_notify ((GObject *)feed, "online");
  } else if (status == TP_CONNECTION_STATUS_DISCONNECTED) {

    /* This gets handled in the notify::status on the connection directly
     * above. We need to do this because of http://bugs.freedesktop.org/show_bug.cgi?id=25149
     */

  } else {
    /* CONNECTING */
  }
}

static void
anerley_tp_feed_setup_tp_connection (AnerleyTpFeed *feed)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  guint res;

  g_signal_connect (priv->account,
                    "status-changed",
                    G_CALLBACK (_account_status_changed_cb),
                    feed);

  res = tp_account_get_connection_status (priv->account, NULL);

  g_debug (G_STRLOC ": Connection is in state: %s",
           res==TP_CONNECTION_STATUS_CONNECTED ? "connected" :
           res==TP_CONNECTION_STATUS_CONNECTING ? "connecting" :
           res==TP_CONNECTION_STATUS_DISCONNECTED ? "disconnected" :
           "other");


  if (res != TP_CONNECTION_STATUS_CONNECTED)
    return;

  _account_status_changed_cb (priv->account,
                              0,
                              TP_CONNECTION_STATUS_CONNECTED,
                              TP_CONNECTION_STATUS_REASON_NONE_SPECIFIED,
                              NULL,
                              NULL,
                              feed);
}

static void
anerley_tp_feed_constructed (GObject *object)
{
  AnerleyTpFeed *feed = (AnerleyTpFeed *)object;

  anerley_tp_feed_setup_tp_connection (feed);

  if (G_OBJECT_CLASS (anerley_tp_feed_parent_class)->constructed)
    G_OBJECT_CLASS (anerley_tp_feed_parent_class)->constructed (object);
}

static void
anerley_tp_feed_class_init (AnerleyTpFeedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyTpFeedPrivate));

  object_class->get_property = anerley_tp_feed_get_property;
  object_class->set_property = anerley_tp_feed_set_property;
  object_class->dispose = anerley_tp_feed_dispose;
  object_class->finalize = anerley_tp_feed_finalize;
  object_class->constructed = anerley_tp_feed_constructed;

  pspec = g_param_spec_object ("account",
                               "account",
                               "The Telepathy account to use",
                               TP_TYPE_ACCOUNT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_ACCOUNT, pspec);

  pspec = g_param_spec_boolean ("online",
                                "online",
                                "Whether we are online or not",
                                FALSE,
                                G_PARAM_READABLE);
  g_object_class_install_property (object_class, PROP_ONLINE, pspec);
}

static void
anerley_tp_feed_init (AnerleyTpFeed *self)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (self);
  gchar *avatar_cache_dir;

  priv->ids_to_items = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              g_free,
                                              (GDestroyNotify)g_object_unref);

  priv->handles_to_ids = g_hash_table_new_full (g_direct_hash,
                                                g_direct_equal,
                                                NULL,
                                                g_free);

  priv->handles_to_contacts = g_hash_table_new_full (g_direct_hash,
                                                     g_direct_equal,
                                                     NULL,
                                                     (GDestroyNotify)g_object_unref);

  /*  Create cache directory if it doesn't exist */
  avatar_cache_dir = g_build_filename (g_get_user_cache_dir (),
                                       "anerley",
                                       "avatars",
                                       NULL);
  g_mkdir_with_parents (avatar_cache_dir, 0755);
  g_free (avatar_cache_dir);
}

AnerleyTpFeed *
anerley_tp_feed_new (TpAccount *account)
{
  return g_object_new (ANERLEY_TYPE_TP_FEED, 
                       "account",
                       account,
                       NULL);
}


static void
feed_interface_init (gpointer g_iface,
                     gpointer iface_data)
{
  /* Nothing to do here..? */
}

AnerleyItem *
anerley_tp_feed_get_item_by_uid (AnerleyTpFeed *feed,
                                 const gchar   *uid)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);

  return g_hash_table_lookup (priv->ids_to_items, uid);
}


gboolean
anerley_tp_feed_get_online (AnerleyTpFeed *feed)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  TpConnectionStatus status;

  if (priv->conn)
  {
    status = tp_account_get_connection_status (priv->account, NULL);
    if (status == TP_CONNECTION_STATUS_CONNECTED)
      return TRUE;
  }

  return FALSE;
}

gboolean
anerley_tp_feed_get_enabled (AnerleyTpFeed *feed)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);

  return tp_account_is_enabled (priv->account);
}
