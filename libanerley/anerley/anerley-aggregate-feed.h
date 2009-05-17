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


#ifndef _ANERLEY_AGGREGATE_FEED
#define _ANERLEY_AGGREGATE_FEED

#include <glib-object.h>
#include <anerley/anerley-feed.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_AGGREGATE_FEED anerley_aggregate_feed_get_type()

#define ANERLEY_AGGREGATE_FEED(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANERLEY_TYPE_AGGREGATE_FEED, AnerleyAggregateFeed))

#define ANERLEY_AGGREGATE_FEED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), ANERLEY_TYPE_AGGREGATE_FEED, AnerleyAggregateFeedClass))

#define ANERLEY_IS_AGGREGATE_FEED(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANERLEY_TYPE_AGGREGATE_FEED))

#define ANERLEY_IS_AGGREGATE_FEED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANERLEY_TYPE_AGGREGATE_FEED))

#define ANERLEY_AGGREGATE_FEED_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANERLEY_TYPE_AGGREGATE_FEED, AnerleyAggregateFeedClass))

typedef struct {
  GObject parent;
} AnerleyAggregateFeed;

typedef struct {
  GObjectClass parent_class;
} AnerleyAggregateFeedClass;

GType anerley_aggregate_feed_get_type (void);

AnerleyFeed *anerley_aggregate_feed_new (void);

void anerley_aggregate_feed_add_feed (AnerleyAggregateFeed *aggregate,
                                      AnerleyFeed          *feed);
void anerley_aggregate_feed_remove_feed (AnerleyAggregateFeed *aggregate,
                                         AnerleyFeed          *feed);

G_END_DECLS

#endif /* _ANERLEY_AGGREGATE_FEED */

