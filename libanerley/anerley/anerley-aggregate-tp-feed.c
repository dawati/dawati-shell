#include "anerley-aggregate-tp-feed.h"

#include <anerley/anerley-tp-feed.h>
#include <libmissioncontrol/mc-account-monitor.h>

G_DEFINE_TYPE (AnerleyAggregateTpFeed, anerley_aggregate_tp_feed, ANERLEY_TYPE_AGGREGATE_FEED)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_AGGREGATE_TP_FEED, AnerleyAggregateTpFeedPrivate))

typedef struct _AnerleyAggregateTpFeedPrivate AnerleyAggregateTpFeedPrivate;

struct _AnerleyAggregateTpFeedPrivate {
  MissionControl *mc;
  McAccountMonitor *account_monitor;
  GHashTable *feeds;
};

enum
{
  PROP_0,
  PROP_MC
};

static void
anerley_aggregate_tp_feed_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_MC:
      g_value_set_object (value, priv->mc);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_aggregate_tp_feed_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_MC:
      priv->mc = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_aggregate_tp_feed_dispose (GObject *object)
{
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (object);

  if (priv->feeds)
  {
    g_hash_table_unref (priv->feeds);
    priv->feeds = NULL;
  }

  if (priv->mc)
  {
    g_object_unref (priv->mc);
    priv->mc = NULL;
  }

  if (priv->account_monitor)
  {
    g_object_unref (priv->account_monitor);
    priv->account_monitor = NULL;
  }

  G_OBJECT_CLASS (anerley_aggregate_tp_feed_parent_class)->dispose (object);
}

static void
anerley_aggregate_tp_feed_finalize (GObject *object)
{
  G_OBJECT_CLASS (anerley_aggregate_tp_feed_parent_class)->finalize (object);
}


static void
anerley_aggregate_tp_feed_constructed (GObject *object)
{
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (object);
  McAccount *account;
  GList *accounts, *l;
  const gchar *account_name;
  AnerleyTpFeed *feed;

  accounts = mc_accounts_list ();

  for (l = accounts; l; l = l->next)
  {
    account = (McAccount *)l->data;
    feed = anerley_tp_feed_new (priv->mc, account);
    account_name = mc_account_get_unique_name (account);
    g_hash_table_insert (priv->feeds,
                         g_strdup (account_name),
                         feed);

    anerley_aggregate_feed_add_feed (ANERLEY_AGGREGATE_FEED (object),
                                     (AnerleyFeed *)feed);
  }

  mc_accounts_list_free (accounts);

  if (G_OBJECT_CLASS (anerley_aggregate_tp_feed_parent_class)->constructed)
    G_OBJECT_CLASS (anerley_aggregate_tp_feed_parent_class)->constructed (object);
}

static void
anerley_aggregate_tp_feed_class_init (AnerleyAggregateTpFeedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyAggregateTpFeedPrivate));

  object_class->get_property = anerley_aggregate_tp_feed_get_property;
  object_class->set_property = anerley_aggregate_tp_feed_set_property;
  object_class->dispose = anerley_aggregate_tp_feed_dispose;
  object_class->finalize = anerley_aggregate_tp_feed_finalize;
  object_class->constructed = anerley_aggregate_tp_feed_constructed;

  pspec = g_param_spec_object ("mc",
                               "Misssion Control",
                               "The mission control instance",
                               MISSIONCONTROL_TYPE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_MC, pspec);
}

static void
_mc_account_monitor_account_enabled (McAccountMonitor *monitor,
                                     gchar            *account_name,
                                     gpointer          userdata)
{
  AnerleyAggregateTpFeed *aggregate = (AnerleyAggregateTpFeed *)userdata;
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (userdata);
  McAccount *account;
  AnerleyTpFeed *feed;

  feed = g_hash_table_lookup (priv->feeds, account_name);

  if (!feed)
  {
    account = mc_account_lookup (account_name);
    feed = anerley_tp_feed_new (priv->mc, account);
    g_object_unref (account);

    g_hash_table_insert (priv->feeds,
                         g_strdup (account_name),
                         feed);

    anerley_aggregate_feed_add_feed (ANERLEY_AGGREGATE_FEED (aggregate),
                                     (AnerleyFeed *)feed);
  }
}

  static void
_mc_account_monitor_account_disabled (McAccountMonitor *monitor,
                                      gchar            *account_name,
                                      gpointer          userdata)
{
  /* For TP feeds we don't actually want to remove the feed from aggregator
   * because otherwise the removals get lost when the TP connection gets
   * disconnect on MC account disablement.
   *
   * If the aggregator becomes cleverer and thus knows which items come from
   * which feed then it can issue the removals and we will then remove the
   * feed here. But for now it isn't.
   */
}



static void
anerley_aggregate_tp_feed_init (AnerleyAggregateTpFeed *self)
{
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (self);

  priv->account_monitor = mc_account_monitor_new ();
  g_signal_connect (priv->account_monitor,
                    "account-enabled",
                    (GCallback)_mc_account_monitor_account_enabled,
                    self);
  g_signal_connect (priv->account_monitor,
                    "account-disabled",
                    (GCallback)_mc_account_monitor_account_disabled,
                    self);

  priv->feeds = g_hash_table_new_full (g_str_hash,
                                       g_str_equal,
                                       g_free,
                                       g_object_unref);
}

AnerleyFeed *
anerley_aggregate_tp_feed_new (MissionControl *mc)
{
  return g_object_new (ANERLEY_TYPE_AGGREGATE_TP_FEED,
                       "mc",
                       mc,
                       NULL);
}


