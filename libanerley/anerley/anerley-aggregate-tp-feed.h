#ifndef _ANERLEY_AGGREGATE_TP_FEED
#define _ANERLEY_AGGREGATE_TP_FEED

#include <glib-object.h>
#include <anerley/anerley-aggregate-feed.h>
#include <libmissioncontrol/mission-control.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_AGGREGATE_TP_FEED anerley_aggregate_tp_feed_get_type()

#define ANERLEY_AGGREGATE_TP_FEED(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANERLEY_TYPE_AGGREGATE_TP_FEED, AnerleyAggregateTpFeed))

#define ANERLEY_AGGREGATE_TP_FEED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), ANERLEY_TYPE_AGGREGATE_TP_FEED, AnerleyAggregateTpFeedClass))

#define ANERLEY_IS_AGGREGATE_TP_FEED(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANERLEY_TYPE_AGGREGATE_TP_FEED))

#define ANERLEY_IS_AGGREGATE_TP_FEED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANERLEY_TYPE_AGGREGATE_TP_FEED))

#define ANERLEY_AGGREGATE_TP_FEED_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANERLEY_TYPE_AGGREGATE_TP_FEED, AnerleyAggregateTpFeedClass))

typedef struct {
  AnerleyAggregateFeed parent;
} AnerleyAggregateTpFeed;

typedef struct {
  AnerleyAggregateFeedClass parent_class;
} AnerleyAggregateTpFeedClass;

GType anerley_aggregate_tp_feed_get_type (void);

AnerleyFeed *anerley_aggregate_tp_feed_new (MissionControl *mc);

G_END_DECLS

#endif /* _ANERLEY_AGGREGATE_TP_FEED */
