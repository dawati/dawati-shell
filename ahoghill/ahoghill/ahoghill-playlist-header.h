#ifndef __AHOGHILL_PLAYLIST_HEADER_H__
#define __AHOGHILL_PLAYLIST_HEADER_H__

#include <nbtk/nbtk.h>


G_BEGIN_DECLS

#define AHOGHILL_TYPE_PLAYLIST_HEADER                                   \
   (ahoghill_playlist_header_get_type())
#define AHOGHILL_PLAYLIST_HEADER(obj)                                   \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_PLAYLIST_HEADER,          \
                                AhoghillPlaylistHeader))
#define AHOGHILL_PLAYLIST_HEADER_CLASS(klass)                           \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_PLAYLIST_HEADER,             \
                             AhoghillPlaylistHeaderClass))
#define IS_AHOGHILL_PLAYLIST_HEADER(obj)                                \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_PLAYLIST_HEADER))
#define IS_AHOGHILL_PLAYLIST_HEADER_CLASS(klass)                        \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_PLAYLIST_HEADER))
#define AHOGHILL_PLAYLIST_HEADER_GET_CLASS(obj)                         \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_PLAYLIST_HEADER,           \
                               AhoghillPlaylistHeaderClass))

typedef struct _AhoghillPlaylistHeaderPrivate AhoghillPlaylistHeaderPrivate;
typedef struct _AhoghillPlaylistHeader      AhoghillPlaylistHeader;
typedef struct _AhoghillPlaylistHeaderClass AhoghillPlaylistHeaderClass;

struct _AhoghillPlaylistHeader
{
    NbtkTable parent;

    AhoghillPlaylistHeaderPrivate *priv;
};

struct _AhoghillPlaylistHeaderClass
{
    NbtkTableClass parent_class;
};

GType ahoghill_playlist_header_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __AHOGHILL_PLAYLIST_HEADER_H__ */
