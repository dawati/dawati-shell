#ifndef _ANERLEY_TP_FEED
#define _ANERLEY_TP_FEED

#include <glib-object.h>
#include <libmissioncontrol/mission-control.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_TP_FEED anerley_tp_feed_get_type()

#define ANERLEY_TP_FEED(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANERLEY_TYPE_TP_FEED, AnerleyTpFeed))

#define ANERLEY_TP_FEED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), ANERLEY_TYPE_TP_FEED, AnerleyTpFeedClass))

#define ANERLEY_IS_TP_FEED(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANERLEY_TYPE_TP_FEED))

#define ANERLEY_IS_TP_FEED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANERLEY_TYPE_TP_FEED))

#define ANERLEY_TP_FEED_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANERLEY_TYPE_TP_FEED, AnerleyTpFeedClass))

typedef struct {
  GObject parent;
} AnerleyTpFeed;

typedef struct {
  GObjectClass parent_class;
} AnerleyTpFeedClass;

GType anerley_tp_feed_get_type (void);

AnerleyTpFeed *anerley_tp_feed_new (MissionControl *mc,
                                    McAccount      *account);

G_END_DECLS

#endif /* _ANERLEY_TP_FEED */
