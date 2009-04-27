#include <libmissioncontrol/mission-control.h>
#include <telepathy-glib/contact.h>

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
  MissionControl *mc;
  McAccount *account;
  TpConnection *conn;
  TpChannel *subscribe_channel;

  GHashTable *ids_to_items;
  GHashTable *handles_to_ids;
};

enum
{
  PROP_0,
  PROP_MC,
  PROP_ACCOUNT
};

static void
anerley_tp_feed_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_MC:
      g_value_set_object (value, priv->mc);
      break;
    case PROP_ACCOUNT:
      g_value_set_object (value, priv->account);
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
    case PROP_MC:
      priv->mc = g_value_dup_object (value);
      break;
    case PROP_ACCOUNT:
      priv->account = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tp_feed_dispose (GObject *object)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (object);

  /*
   * Do here rather than finalize since this will then cause the TpContact
   * inside the item to be unreffed.
   */
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
    g_object_unref (priv->account);
    priv->account = NULL;
  }

  if (priv->mc)
  {
    g_object_unref (priv->mc);
    priv->mc = NULL;
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
  gchar *uid;
  GList *changed_items = NULL;
  GList *added_items = NULL;
  gchar *avatar_path = NULL;
  GArray *missing_avatar_handles;
  const gchar *token;
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

    g_debug (G_STRLOC ": Got contact: %s (%s) - %s",
             tp_contact_get_alias (contact),
             tp_contact_get_identifier (contact),
             tp_contact_get_presence_status (contact));

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
      uid = g_strdup_printf ("%s/%s",
                             mc_account_get_normalized_name (priv->account),
                             tp_contact_get_identifier (contact));
      item = anerley_tp_item_new (contact);

      added_items = g_list_append (added_items, item);

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
    }

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
        /* Already download, woot! */
        anerley_tp_item_set_avatar_path (item, avatar_path);
      } else {
        handle = tp_contact_get_handle (contact);
        g_array_append_val (missing_avatar_handles,
                            handle);
      }

      g_free (avatar_path);
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

  tp_cli_connection_interface_avatars_call_request_avatars (priv->conn,
                                                            -1,
                                                            missing_avatar_handles,
                                                            _tp_connection_request_avatars_cb,
                                                            NULL,
                                                            NULL,
                                                            (GObject *)feed);
  g_array_free (missing_avatar_handles, TRUE);
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

  g_debug (G_STRLOC ": Members changed.");
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
_mc_account_status_changed_cb (MissionControl           *mc,
                               TpConnectionStatus        status,
                               McPresence                presence,
                               TpConnectionStatusReason  reason,
                               const gchar              *account_name,
                               gpointer                  userdata)
{
  AnerleyTpFeed *feed = (AnerleyTpFeed *)userdata;
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  GList *items;
  GError *error = NULL;

  /* This is so lame. I hear this should get better with MC5 though */
  if (g_str_equal (account_name, mc_account_get_unique_name (priv->account)))
  {
    if (status == TP_CONNECTION_STATUS_CONNECTED)
    {
      priv->conn = mission_control_get_tpconnection (priv->mc,
                                                     priv->account,
                                                     &error);
      if (!priv->conn)
      {
        g_warning (G_STRLOC ": Error getting TP connection: %s",
                   error->message);
        g_clear_error (&error);
        return;
      }

      tp_connection_call_when_ready (priv->conn,
                                     _tp_connection_ready_cb,
                                     feed);
    } else {
      /*
       * This means our connection has been disconnected. That is :-( so lets
       * remove these things from our internal store and emit the signals on
       * the feed.
       */

      items = g_hash_table_get_values (priv->ids_to_items);

      g_signal_emit_by_name (feed, "items-removed", items);
      g_list_free (items);
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
    }
  }
}

static void
anerley_tp_feed_setup_tp_connection (AnerleyTpFeed *feed)
{
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);
  guint res;
  GError *error = NULL;

  /* 
   * Check the connection status. Now, in theory we should have only been
   * created for an online connection however let's check anyway and warn out
   * if this has happened
   */

  res = mission_control_get_connection_status (priv->mc,
                                               priv->account,
                                               &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error requesting connection status: %s",
               error->message);
    g_clear_error (&error);
    return;
  }

  dbus_g_proxy_connect_signal (DBUS_G_PROXY (priv->mc),
                               "AccountStatusChanged",
                               G_CALLBACK (_mc_account_status_changed_cb),
                               feed,
                               NULL);

  g_debug (G_STRLOC ": Connection is in state: %s",
           res==TP_CONNECTION_STATUS_CONNECTED ? "connected" :
           res==TP_CONNECTION_STATUS_CONNECTING ? "connecting" :
           res==TP_CONNECTION_STATUS_DISCONNECTED ? "disconnected" :
           "other");

  if (res != TP_CONNECTION_STATUS_CONNECTED)
    return;

  /* 
   * Since we've established that the connection is either connected or
   * connecting then there will a connection available for us to grab
   */
  priv->conn = mission_control_get_tpconnection (priv->mc,
                                                 priv->account,
                                                 &error);
  if (!priv->conn)
  {
    g_warning (G_STRLOC ": Error getting TP connection: %s",
               error->message);
    g_clear_error (&error);
    return;
  }

  /* 
   * The connection may or may not be ready, let's get ourselves called when
   * it is.
   */
  tp_connection_call_when_ready (priv->conn,
                                 _tp_connection_ready_cb,
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

  pspec = g_param_spec_object ("mc",
                               "mission control",
                               "The mission control object",
                               MISSIONCONTROL_TYPE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_MC, pspec);

  pspec = g_param_spec_object ("account",
                               "account",
                               "The mission control account to use",
                               MC_TYPE_ACCOUNT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_ACCOUNT, pspec);
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

  /*  Create cache directory if it doesn't exist */
  avatar_cache_dir = g_build_filename (g_get_user_cache_dir (),
                                       "anerley",
                                       "avatars",
                                       NULL);
  g_mkdir_with_parents (avatar_cache_dir, 0755);
  g_free (avatar_cache_dir);
}

AnerleyTpFeed *
anerley_tp_feed_new (MissionControl *mc,
                     McAccount      *account)
{
  return g_object_new (ANERLEY_TYPE_TP_FEED, 
                       "mc",
                       mc,
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
