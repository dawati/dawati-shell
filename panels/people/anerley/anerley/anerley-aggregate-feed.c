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


#include "anerley-aggregate-feed.h"

static void feed_interface_init (gpointer g_iface, gpointer iface_data);
G_DEFINE_TYPE_WITH_CODE (AnerleyAggregateFeed,
                         anerley_aggregate_feed,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (ANERLEY_TYPE_FEED,
                                                feed_interface_init));

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_AGGREGATE_FEED, AnerleyAggregateFeedPrivate))

typedef struct _AnerleyAggregateFeedPrivate AnerleyAggregateFeedPrivate;

struct _AnerleyAggregateFeedPrivate {
  GList *feeds;
};

static void
anerley_aggregate_feed_dispose (GObject *object)
{
  AnerleyAggregateFeedPrivate *priv = GET_PRIVATE (object);
  GList *l;
  AnerleyFeed *feed;

  for (l = priv->feeds; l; l = l->next)
  {
    feed = (AnerleyFeed *)l->data;
    anerley_aggregate_feed_remove_feed ((AnerleyAggregateFeed *)object,
                                        feed);
  }

  G_OBJECT_CLASS (anerley_aggregate_feed_parent_class)->dispose (object);
}

static void
anerley_aggregate_feed_class_init (AnerleyAggregateFeedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AnerleyAggregateFeedPrivate));

  object_class->dispose = anerley_aggregate_feed_dispose;
}

static void
anerley_aggregate_feed_init (AnerleyAggregateFeed *self)
{
}

AnerleyFeed *
anerley_aggregate_feed_new (void)
{
  return g_object_new (ANERLEY_TYPE_AGGREGATE_FEED, NULL);
}

static void
_feed_items_added_cb (AnerleyFeed *feed,
                      GList       *items,
                      gpointer     userdata)
{
  AnerleyAggregateFeed *aggregate = (AnerleyAggregateFeed *)userdata;

  g_signal_emit_by_name (aggregate,
                         "items-added",
                         items);
}

static void
_feed_items_removed_cb (AnerleyFeed *feed,
                        GList       *items,
                        gpointer     userdata)
{
  AnerleyAggregateFeed *aggregate = (AnerleyAggregateFeed *)userdata;

  g_signal_emit_by_name (aggregate,
                         "items-removed",
                         items);
}

void
anerley_aggregate_feed_add_feed (AnerleyAggregateFeed *aggregate,
                                 AnerleyFeed          *feed)
{
  AnerleyAggregateFeedPrivate *priv = GET_PRIVATE (aggregate);

  priv->feeds = g_list_append (priv->feeds, feed);

  g_object_ref (feed);

  g_signal_connect (feed,
                    "items-added",
                    (GCallback)_feed_items_added_cb,
                    aggregate);
  g_signal_connect (feed,
                    "items-removed",
                    (GCallback)_feed_items_removed_cb,
                    aggregate);
}

void
anerley_aggregate_feed_remove_feed (AnerleyAggregateFeed *aggregate,
                                    AnerleyFeed          *feed)
{
  AnerleyAggregateFeedPrivate *priv = GET_PRIVATE (aggregate);

  priv->feeds = g_list_remove (priv->feeds, feed);

  g_signal_handlers_disconnect_by_func (feed,
                                        _feed_items_added_cb,
                                        aggregate);
  g_signal_handlers_disconnect_by_func (feed,
                                        _feed_items_removed_cb,
                                        aggregate);

  g_object_unref (feed);
}

static void
feed_interface_init (gpointer g_iface,
                     gpointer iface_data)
{
  /* Nothing to do here..? */
}

