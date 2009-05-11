#ifndef __AHOGHILL_QUEUE_LIST_H__
#define __AHOGHILL_QUEUE_LIST_H__

#include <nbtk/nbtk.h>


G_BEGIN_DECLS

#define AHOGHILL_TYPE_QUEUE_LIST                                        \
   (ahoghill_queue_list_get_type())
#define AHOGHILL_QUEUE_LIST(obj)                                        \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_QUEUE_LIST,               \
                                AhoghillQueueList))
#define AHOGHILL_QUEUE_LIST_CLASS(klass)                                \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_QUEUE_LIST,                  \
                             AhoghillQueueListClass))
#define IS_AHOGHILL_QUEUE_LIST(obj)                                     \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_QUEUE_LIST))
#define IS_AHOGHILL_QUEUE_LIST_CLASS(klass)                             \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_QUEUE_LIST))
#define AHOGHILL_QUEUE_LIST_GET_CLASS(obj)                              \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_QUEUE_LIST,                \
                               AhoghillQueueListClass))

typedef struct _AhoghillQueueListPrivate AhoghillQueueListPrivate;
typedef struct _AhoghillQueueList      AhoghillQueueList;
typedef struct _AhoghillQueueListClass AhoghillQueueListClass;

struct _AhoghillQueueList
{
    NbtkScrollView parent;

    AhoghillQueueListPrivate *priv;
};

struct _AhoghillQueueListClass
{
    NbtkScrollViewClass parent_class;
};

GType ahoghill_queue_list_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __AHOGHILL_QUEUE_LIST_H__ */
