
#ifndef ANERLEY_FEED_H
#define ANERLEY_FEED_H

#include <glib-object.h>

#define ANERLEY_TYPE_FEED (anerley_feed_get_type ())
#define ANERLEY_FEED(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                           ANERLEY_TYPE_FEED, AnerleyFeed))
#define ANERLEY_IS_FEED(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                              ANERLEY_TYPE_FEED))
#define ANERLEY_FEED_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst),\
                                          ANERLEY_TYPE_FEED, AnerleyFeedInterface))

typedef struct _AnerleyFeed AnerleyFeed;
typedef struct _AnerleyFeedInterface AnerleyFeedInterface;

struct _AnerleyFeedInterface {
  GTypeInterface parent;

  /* No vfuncs ? */
};

GType anerley_feed_get_type (void);

#endif /* ANERLEY_FEED_H */
