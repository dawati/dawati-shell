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

#include "anerley-tp-monitor-feed.h"

#include <anerley/anerley-feed.h>
#include <anerley/anerley-item.h>
#include <anerley/anerley-tp-item.h>
#include <anerley/anerley-tp-feed.h>
#include <anerley/anerley-tp-observer.h>
#include <telepathy-glib/channel.h>

G_DEFINE_TYPE_WITH_CODE (AnerleyTpMonitorFeed,
                         anerley_tp_monitor_feed,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (ANERLEY_TYPE_FEED,
                                                NULL));

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_TP_MONITIOR_FEED, AnerleyTpMonitorFeedPrivate))

typedef struct _AnerleyTpMonitorFeedPrivate AnerleyTpMonitorFeedPrivate;

struct _AnerleyTpMonitorFeedPrivate {
  AnerleyAggregateTpFeed *aggregate_feed;
  AnerleyTpObserver *observer;
  GHashTable *channels_to_items;
  gchar *client_name;
};

enum
{
  PROP_0,
  PROP_AGREGGATE_FEED,
  PROP_CLIENT_NAME
};

static void
anerley_tp_monitor_feed_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_CLIENT_NAME:
      g_value_set_string (value, priv->client_name);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tp_monitor_feed_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_AGREGGATE_FEED:
      priv->aggregate_feed = g_value_dup_object (value);
      break;
    case PROP_CLIENT_NAME:
      priv->client_name = g_value_dup_string (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_tp_monitor_feed_dispose (GObject *object)
{
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (object);

  if (priv->aggregate_feed)
  {
    g_object_unref (priv->aggregate_feed);
    priv->aggregate_feed = NULL;
  }

  if (priv->observer)
  {
    g_object_unref (priv->observer);
    priv->observer = NULL;
  }

  G_OBJECT_CLASS (anerley_tp_monitor_feed_parent_class)->dispose (object);
}

static void
anerley_tp_monitor_feed_finalize (GObject *object)
{
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (object);

  g_free (priv->client_name);
  G_OBJECT_CLASS (anerley_tp_monitor_feed_parent_class)->finalize (object);
}

static void
_channel_closed_cb (TpChannel *channel,
                    gpointer   userdata,
                    GObject   *weak_object)
{
  AnerleyTpMonitorFeed *feed = ANERLEY_TP_MONITIOR_FEED (weak_object);
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (feed);
  GList *items = NULL;
  AnerleyItem *item;

  g_debug (G_STRLOC ": Channel with identifier %s closed",
           tp_channel_get_identifier (channel));

  item = g_hash_table_lookup (priv->channels_to_items, channel);
  items = g_list_append (items, item);
  g_signal_emit_by_name (feed,
                         "items-removed",
                         items);
  g_hash_table_remove (priv->channels_to_items,
                       channel);

  g_object_unref (channel);
}

static void
_observer_new_channel_cb (AnerleyTpObserver *observer,
                          const gchar       *account_name,
                          TpChannel         *channel,
                          gpointer           userdata)
{
  AnerleyTpMonitorFeed *monitor_feed = ANERLEY_TP_MONITIOR_FEED (userdata);
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (userdata);
  AnerleyFeed *feed;
  const gchar *uid;
  AnerleyItem *item;
  GList *items = NULL;

  feed = anerley_aggregate_tp_feed_get_feed_by_account_name (priv->aggregate_feed,
                                                             account_name);

  if (!feed)
  {
    g_warning (G_STRLOC ": Given an account name with no known feed: %s",
               account_name);
    return;
  } else {
    g_debug (G_STRLOC ": Got a valid feed for %s account", account_name);
  }

  uid = tp_channel_get_identifier (channel);
  item = anerley_tp_feed_get_item_by_uid (ANERLEY_TP_FEED (feed),
                                          uid);

  if (!item)
  {
    g_warning (G_STRLOC ": Given a channel with no matching item: %s",
               uid);
    return;
  } else {
    g_debug (G_STRLOC ": Got a valid item for uid %s", uid);
  }

  tp_cli_channel_connect_to_closed (channel,
                                    _channel_closed_cb,
                                    NULL,
                                    NULL,
                                    userdata,
                                    NULL);

  g_hash_table_insert (priv->channels_to_items,
                       channel,
                       g_object_ref (item));

  anerley_tp_item_associate_channel (ANERLEY_TP_ITEM (item), channel);

  items = g_list_append (items, item);
  g_signal_emit_by_name (monitor_feed,
                         "items-added",
                         items);
}

static void
anerley_tp_monitor_feed_constructed (GObject *object)
{
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (object);

  priv->observer = anerley_tp_observer_new (priv->client_name);
  g_signal_connect (priv->observer,
                    "new-channel",
                    (GCallback)_observer_new_channel_cb,
                    object);

  if (G_OBJECT_CLASS (anerley_tp_monitor_feed_parent_class)->constructed)
    G_OBJECT_CLASS (anerley_tp_monitor_feed_parent_class)->constructed (object);
}

static void
anerley_tp_monitor_feed_class_init (AnerleyTpMonitorFeedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyTpMonitorFeedPrivate));

  object_class->get_property = anerley_tp_monitor_feed_get_property;
  object_class->set_property = anerley_tp_monitor_feed_set_property;
  object_class->dispose = anerley_tp_monitor_feed_dispose;
  object_class->finalize = anerley_tp_monitor_feed_finalize;
  object_class->constructed = anerley_tp_monitor_feed_constructed;

  pspec = g_param_spec_object ("aggregate-feed",
                               "Aggregate feed",
                               "The feed to look items up in",
                               ANERLEY_TYPE_AGGREGATE_TP_FEED,
                               G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_AGREGGATE_FEED, pspec);

  pspec = g_param_spec_string ("client-name",
                               "Client name",
                               "Name for the Telepathy client",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_CLIENT_NAME, pspec);
}

static void
anerley_tp_monitor_feed_init (AnerleyTpMonitorFeed *self)
{
  AnerleyTpMonitorFeedPrivate *priv = GET_PRIVATE (self);

  priv->channels_to_items = g_hash_table_new_full (g_direct_hash,
                                                   g_direct_equal,
                                                   NULL,
                                                   g_object_unref);
}

AnerleyFeed *
anerley_tp_monitor_feed_new (AnerleyAggregateTpFeed *aggregate,
                             const gchar            *client_name)
{
  return g_object_new (ANERLEY_TYPE_TP_MONITIOR_FEED,
                       "aggregate-feed", aggregate,
                       "client-name", client_name,
                       NULL);
}


