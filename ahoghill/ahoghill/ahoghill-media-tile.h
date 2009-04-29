#ifndef __AHOGHILL_MEDIA_TILE_H__
#define __AHOGHILL_MEDIA_TILE_H__

#include <nbtk/nbtk.h>


G_BEGIN_DECLS

#define AHOGHILL_TYPE_MEDIA_TILE                                        \
   (ahoghill_media_tile_get_type())
#define AHOGHILL_MEDIA_TILE(obj)                                        \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_MEDIA_TILE,               \
                                AhoghillMediaTile))
#define AHOGHILL_MEDIA_TILE_CLASS(klass)                                \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_MEDIA_TILE,                  \
                             AhoghillMediaTileClass))
#define IS_AHOGHILL_MEDIA_TILE(obj)                                     \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_MEDIA_TILE))
#define IS_AHOGHILL_MEDIA_TILE_CLASS(klass)                             \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_MEDIA_TILE))
#define AHOGHILL_MEDIA_TILE_GET_CLASS(obj)                              \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_MEDIA_TILE,                \
                               AhoghillMediaTileClass))

typedef struct _AhoghillMediaTilePrivate AhoghillMediaTilePrivate;
typedef struct _AhoghillMediaTile      AhoghillMediaTile;
typedef struct _AhoghillMediaTileClass AhoghillMediaTileClass;

struct _AhoghillMediaTile
{
    NbtkTable parent;

    AhoghillMediaTilePrivate *priv;
};

struct _AhoghillMediaTileClass
{
    NbtkTableClass parent_class;
};

GType ahoghill_media_tile_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __AHOGHILL_MEDIA_TILE_H__ */
