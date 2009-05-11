#ifndef __AHOGHILL_PLAYLIST_H__
#define __AHOGHILL_PLAYLIST_H__

#include <nbtk/nbtk.h>
#include <ahoghill/ahoghill-queue-list.h>
#include <ahoghill/ahoghill-grid-view.h>

G_BEGIN_DECLS

#define AHOGHILL_TYPE_PLAYLIST                                          \
   (ahoghill_playlist_get_type())
#define AHOGHILL_PLAYLIST(obj)                                          \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_PLAYLIST,                 \
                                AhoghillPlaylist))
#define AHOGHILL_PLAYLIST_CLASS(klass)                                  \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_PLAYLIST,                    \
                             AhoghillPlaylistClass))
#define IS_AHOGHILL_PLAYLIST(obj)                                       \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_PLAYLIST))
#define IS_AHOGHILL_PLAYLIST_CLASS(klass)                               \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_PLAYLIST))
#define AHOGHILL_PLAYLIST_GET_CLASS(obj)                                \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_PLAYLIST,                  \
                               AhoghillPlaylistClass))

typedef struct _AhoghillPlaylistPrivate AhoghillPlaylistPrivate;
typedef struct _AhoghillPlaylist      AhoghillPlaylist;
typedef struct _AhoghillPlaylistClass AhoghillPlaylistClass;

struct _AhoghillPlaylist
{
    NbtkTable parent;

    AhoghillPlaylistPrivate *priv;
};

struct _AhoghillPlaylistClass
{
    NbtkTableClass parent_class;
};

GType ahoghill_playlist_get_type (void) G_GNUC_CONST;
AhoghillPlaylist *ahoghill_playlist_new (AhoghillGridView *view,
                                         const char       *title);
void ahoghill_playlist_set_queue (AhoghillPlaylist *playlist,
                                  BrQueue          *queue);

AhoghillQueueList *ahoghill_playlist_get_queue_list (AhoghillPlaylist *playlist);
G_END_DECLS

#endif /* __AHOGHILL_PLAYLIST_H__ */
