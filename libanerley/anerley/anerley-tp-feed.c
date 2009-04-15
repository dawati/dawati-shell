#include <libmissioncontrol/mission-control.h>
#include <telepathy-glib/contact.h>

#include "anerley-feed.h"
#include "anerley-tp-feed.h"

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
};

enum
{
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

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

  if (error)
  {
    g_warning (G_STRLOC ": Error when fetching contacts: %s",
               error->message);
    return;
  }

  for (i = 0; i < n_contacts; i++)
  {
    contact = (TpContact *)contacts[i];

    g_debug (G_STRLOC ": Got contact: %s (%s) - %s",
             tp_contact_get_alias (contact),
             tp_contact_get_identifier (contact),
             tp_contact_get_presence_status (contact));
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

  g_debug (G_STRLOC ": Members changed.");
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
  AnerleyTpFeedPrivate *priv = GET_PRIVATE (feed);

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
_tp_connection_ready_cb (TpConnection *connection,
                         const GError *error,
                         gpointer      userdata)
{
  AnerleyTpFeed *feed = (AnerleyTpFeed *)userdata;

  if (error)
  {
    g_warning (G_STRLOC ": Error. Connection is now invalid: %s",
               error->message);
    return;
  }

  /* The next thing we care about is getting the list of contacts we subscribe
   * to. That requires us to request and set up an appropriate channel.
   */

  anerley_tp_feed_setup_subscribe_channel (feed);
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

  g_debug (G_STRLOC ": Connection is in state: %s",
           res==0 ? "connected" :
           res==1 ? "connecting" :
           res==2 ? "disconnected" :
           "other");

  if (res >= 2)
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
