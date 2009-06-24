#ifndef __AHOGHILL_PLAYLIST_NP_H__
#define __AHOGHILL_PLAYLIST_NP_H__

#include <nbtk/nbtk.h>
#include <bickley/bkl-item.h>

G_BEGIN_DECLS

#define AHOGHILL_TYPE_PLAYLIST_NP                                       \
   (ahoghill_playlist_np_get_type())
#define AHOGHILL_PLAYLIST_NP(obj)                                       \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_PLAYLIST_NP,              \
                                AhoghillPlaylistNp))
#define AHOGHILL_PLAYLIST_NP_CLASS(klass)                               \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_PLAYLIST_NP,                 \
                             AhoghillPlaylistNpClass))
#define IS_AHOGHILL_PLAYLIST_NP(obj)                                    \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_PLAYLIST_NP))
#define IS_AHOGHILL_PLAYLIST_NP_CLASS(klass)                            \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_PLAYLIST_NP))
#define AHOGHILL_PLAYLIST_NP_GET_CLASS(obj)                             \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_PLAYLIST_NP,               \
                               AhoghillPlaylistNpClass))

typedef struct _AhoghillPlaylistNpPrivate AhoghillPlaylistNpPrivate;
typedef struct _AhoghillPlaylistNp      AhoghillPlaylistNp;
typedef struct _AhoghillPlaylistNpClass AhoghillPlaylistNpClass;

struct _AhoghillPlaylistNp
{
    NbtkTable parent;

    AhoghillPlaylistNpPrivate *priv;
};

struct _AhoghillPlaylistNpClass
{
    NbtkTableClass parent_class;
};

GType ahoghill_playlist_np_get_type (void) G_GNUC_CONST;
void ahoghill_playlist_np_set_item (AhoghillPlaylistNp *now_playing,
                                    BklItem            *item);
void ahoghill_playlist_np_set_position (AhoghillPlaylistNp *now_playing,
                                        double              position);

G_END_DECLS

#endif /* __AHOGHILL_PLAYLIST_NP_H__ */
