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


#include "anerley-aggregate-tp-feed.h"

#include <anerley/anerley-tp-feed.h>

#include <telepathy-glib/account-manager.h>

G_DEFINE_TYPE (AnerleyAggregateTpFeed, anerley_aggregate_tp_feed, ANERLEY_TYPE_AGGREGATE_FEED)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_AGGREGATE_TP_FEED, AnerleyAggregateTpFeedPrivate))

typedef struct _AnerleyAggregateTpFeedPrivate AnerleyAggregateTpFeedPrivate;

struct _AnerleyAggregateTpFeedPrivate {
  GHashTable *feeds;
  TpAccountManager *account_manager;
};

enum
{
  PROP_0,
  PROP_ACCOUNTS_AVAILABLE,
  PROP_ACCOUNTS_ONLINE
};

static void
anerley_aggregate_tp_feed_dispose (GObject *object)
{
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (object);

  if (priv->feeds)
  {
    g_hash_table_unref (priv->feeds);
    priv->feeds = NULL;
  }

  if (priv->account_manager != NULL)
  {
    g_object_unref (priv->account_manager);
    priv->account_manager = NULL;
  }

  G_OBJECT_CLASS (anerley_aggregate_tp_feed_parent_class)->dispose (object);
}

static void
anerley_aggregate_tp_feed_finalize (GObject *object)
{
  G_OBJECT_CLASS (anerley_aggregate_tp_feed_parent_class)->finalize (object);
}

static void
anerley_aggregate_tp_feed_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AnerleyAggregateTpFeed *feed = ANERLEY_AGGREGATE_TP_FEED (object);

  switch (property_id) {
    case PROP_ACCOUNTS_AVAILABLE:
      g_value_set_int (value,
                       anerley_aggregate_tp_feed_get_accounts_available (feed));
      break;
    case PROP_ACCOUNTS_ONLINE:
      g_value_set_int (value,
                       anerley_aggregate_tp_feed_get_accounts_online (feed));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_aggregate_tp_feed_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_feed_notify_online_cb (GObject    *object,
                        GParamSpec *pspec,
                        gpointer    userdata)
{
  g_object_notify (G_OBJECT (userdata), "accounts-online");
}

static void
_add_feed_from_account (AnerleyAggregateTpFeed *aggregate,
                        TpAccount              *account)
{
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (aggregate);
  AnerleyTpFeed *feed;
  const gchar *account_name;

  account_name = tp_proxy_get_object_path (account);

  feed = g_hash_table_lookup (priv->feeds, account_name);

  if (!feed)
  {
    feed = anerley_tp_feed_new (account);

    g_hash_table_insert (priv->feeds,
                         g_strdup (account_name),
                         feed);

    g_debug (G_STRLOC ": Adding account with identifier %s to feed",
             account_name);

    anerley_aggregate_feed_add_feed (ANERLEY_AGGREGATE_FEED (aggregate),
                                     ANERLEY_FEED (feed));
    g_signal_connect (feed,
                      "notify::online",
                      (GCallback)_feed_notify_online_cb,
                      aggregate);
    g_object_notify (G_OBJECT (aggregate), "accounts-available");
    g_object_notify (G_OBJECT (aggregate), "accounts-online");
  }
}

static void
_account_manager_account_validity_changed_cb (TpAccountManager *account_manager,
                                              TpAccount        *account,
                                              gboolean          valid,
                                              gpointer          userdata)
{
  AnerleyAggregateTpFeed *aggregate = ANERLEY_AGGREGATE_TP_FEED (userdata);

  if (!valid)
    return;

  _add_feed_from_account (aggregate, account);
}

static void
_account_manager_account_enabled_cb (TpAccountManager *account_manager,
                                     TpAccount        *account,
                                     gpointer          userdata)
{
  AnerleyAggregateTpFeed *aggregate = ANERLEY_AGGREGATE_TP_FEED (userdata);

  _add_feed_from_account (aggregate, account);

  g_object_notify (G_OBJECT (userdata), "accounts-available");
  g_object_notify (G_OBJECT (userdata), "accounts-online");
}

static void
_account_manager_account_disabled_cb (TpAccountManager *account_manager,
                                      TpAccount        *account,
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

  g_object_notify (G_OBJECT (userdata), "accounts-available");
  g_object_notify (G_OBJECT (userdata), "accounts-online");
}

static void
_account_manager_account_removed_cb (TpAccountManager *account_manager,
                                     TpAccount        *account,
                                     gpointer          userdata)
{
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (userdata);

  /* For TP feeds we don't actually want to remove the feed from aggregator
   * because otherwise the removals get lost when the TP connection gets
   * disconnect on MC account disablement.
   *
   * If the aggregator becomes cleverer and thus knows which items come from
   * which feed then it can issue the removals and we will then remove the
   * feed here. But for now it isn't.
   */

  g_object_notify (G_OBJECT (userdata), "accounts-online");
  g_object_notify (G_OBJECT (userdata), "accounts-available");

  g_hash_table_remove (priv->feeds,
                       tp_proxy_get_object_path (TP_PROXY (account)));
}


static void
_account_manager_ready_cb (GObject      *source_object,
                           GAsyncResult *result,
                           gpointer      userdata)
{
  TpAccountManager *account_manager = TP_ACCOUNT_MANAGER (source_object);
  AnerleyAggregateTpFeed *aggregate = ANERLEY_AGGREGATE_TP_FEED (userdata);
  GList *accounts, *l;
  GError *error = NULL;

  if (!tp_account_manager_prepare_finish (account_manager, result, &error))
  {
    g_warning ("Failed to prepare account manager: %s", error->message);
    g_error_free (error);
    return;
  }

  accounts = tp_account_manager_get_valid_accounts (account_manager);

  for (l = accounts; l != NULL; l = l->next)
  {
    TpAccount *account = TP_ACCOUNT (l->data);

    _add_feed_from_account (aggregate, account);
  }

  g_list_free (accounts);
}

static void
anerley_aggregate_tp_feed_constructed (GObject *object)
{
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (object);

  priv->account_manager = tp_account_manager_dup ();
  g_signal_connect (priv->account_manager,
                    "account-enabled",
                    G_CALLBACK (_account_manager_account_enabled_cb),
                    object);
  g_signal_connect (priv->account_manager,
                    "account-disabled",
                    G_CALLBACK (_account_manager_account_disabled_cb),
                    object);
  g_signal_connect (priv->account_manager,
                    "account-validity-changed",
                    G_CALLBACK (_account_manager_account_validity_changed_cb),
                    object);
  g_signal_connect (priv->account_manager,
                    "account-removed",
                    G_CALLBACK (_account_manager_account_removed_cb),
                    object);

  tp_account_manager_prepare_async (priv->account_manager,
                                    NULL,
                                    _account_manager_ready_cb,
                                    object);

  if (G_OBJECT_CLASS (anerley_aggregate_tp_feed_parent_class)->constructed)
    G_OBJECT_CLASS (anerley_aggregate_tp_feed_parent_class)->constructed (object);
}

static void
anerley_aggregate_tp_feed_class_init (AnerleyAggregateTpFeedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyAggregateTpFeedPrivate));

  object_class->dispose = anerley_aggregate_tp_feed_dispose;
  object_class->finalize = anerley_aggregate_tp_feed_finalize;
  object_class->constructed = anerley_aggregate_tp_feed_constructed;
  object_class->get_property = anerley_aggregate_tp_feed_get_property;
  object_class->set_property = anerley_aggregate_tp_feed_set_property;

  pspec = g_param_spec_int ("accounts-available",
                            "accounts-available",
                            "Number of accounts available",
                            0,
                            G_MAXINT,
                            0,
                            G_PARAM_READABLE);
  g_object_class_install_property (object_class, PROP_ACCOUNTS_AVAILABLE, pspec);

  pspec = g_param_spec_int ("accounts-online",
                            "accounts-online",
                            "Number of accounts online",
                            0,
                            G_MAXINT,
                            0,
                            G_PARAM_READABLE);
  g_object_class_install_property (object_class, PROP_ACCOUNTS_ONLINE, pspec);
}

static void
anerley_aggregate_tp_feed_init (AnerleyAggregateTpFeed *self)
{
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (self);

  priv->feeds = g_hash_table_new_full (g_str_hash,
                                       g_str_equal,
                                       g_free,
                                       g_object_unref);
}

AnerleyFeed *
anerley_aggregate_tp_feed_new (void)
{
  return g_object_new (ANERLEY_TYPE_AGGREGATE_TP_FEED, NULL);
}

AnerleyFeed *
anerley_aggregate_tp_feed_get_feed_by_account_name (AnerleyAggregateTpFeed *feed,
                                                    const gchar            *account_name)
{
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (feed);

  return g_hash_table_lookup (priv->feeds, account_name);
}

gint
anerley_aggregate_tp_feed_get_accounts_online (AnerleyAggregateTpFeed *feed)
{
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (feed);
  GList *feeds = NULL;
  gint online = 0;
  GList *l;

  feeds = g_hash_table_get_values (priv->feeds);

  for (l = feeds; l; l = l->next)
  {
    if (anerley_tp_feed_get_online (ANERLEY_TP_FEED (l->data)))
    {
      online++;
    }
  }

  g_list_free (feeds);

  return online;
}

gint
anerley_aggregate_tp_feed_get_accounts_available (AnerleyAggregateTpFeed *feed)
{
  AnerleyAggregateTpFeedPrivate *priv = GET_PRIVATE (feed);
  GList *feeds = NULL;
  gint available = 0;
  GList *l;

  feeds = g_hash_table_get_values (priv->feeds);

  for (l = feeds; l; l = l->next)
  {
    if (anerley_tp_feed_get_enabled (ANERLEY_TP_FEED (l->data)))
      available++;
  }

  g_list_free (feeds);

  return available;
}
