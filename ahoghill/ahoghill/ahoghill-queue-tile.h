#ifndef __AHOGHILL_QUEUE_TILE_H__
#define __AHOGHILL_QUEUE_TILE_H__

#include <bickley/bkl.h>
#include <nbtk/nbtk.h>


G_BEGIN_DECLS

#define AHOGHILL_TYPE_QUEUE_TILE                                        \
   (ahoghill_queue_tile_get_type())
#define AHOGHILL_QUEUE_TILE(obj)                                        \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_QUEUE_TILE,               \
                                AhoghillQueueTile))
#define AHOGHILL_QUEUE_TILE_CLASS(klass)                                \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_QUEUE_TILE,                  \
                             AhoghillQueueTileClass))
#define IS_AHOGHILL_QUEUE_TILE(obj)                                     \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_QUEUE_TILE))
#define IS_AHOGHILL_QUEUE_TILE_CLASS(klass)                             \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_QUEUE_TILE))
#define AHOGHILL_QUEUE_TILE_GET_CLASS(obj)                              \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_QUEUE_TILE,                \
                               AhoghillQueueTileClass))

typedef struct _AhoghillQueueTilePrivate AhoghillQueueTilePrivate;
typedef struct _AhoghillQueueTile      AhoghillQueueTile;
typedef struct _AhoghillQueueTileClass AhoghillQueueTileClass;

struct _AhoghillQueueTile
{
    NbtkTable parent;

    AhoghillQueueTilePrivate *priv;
};

struct _AhoghillQueueTileClass
{
    NbtkTableClass parent_class;
};

GType ahoghill_queue_tile_get_type (void) G_GNUC_CONST;

void ahoghill_queue_tile_set_item (AhoghillQueueTile *tile,
                                   BklItem           *item);

G_END_DECLS

#endif /* __AHOGHILL_QUEUE_TILE_H__ */
