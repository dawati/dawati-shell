#ifndef __AHOGHILL_PLAYLIST_PLACEHOLDER_H__
#define __AHOGHILL_PLAYLIST_PLACEHOLDER_H__

#include <nbtk/nbtk.h>


G_BEGIN_DECLS

#define AHOGHILL_TYPE_PLAYLIST_PLACEHOLDER                              \
   (ahoghill_playlist_placeholder_get_type())
#define AHOGHILL_PLAYLIST_PLACEHOLDER(obj)                              \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_PLAYLIST_PLACEHOLDER,     \
                                AhoghillPlaylistPlaceholder))
#define AHOGHILL_PLAYLIST_PLACEHOLDER_CLASS(klass)                      \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_PLAYLIST_PLACEHOLDER,        \
                             AhoghillPlaylistPlaceholderClass))
#define IS_AHOGHILL_PLAYLIST_PLACEHOLDER(obj)                           \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_PLAYLIST_PLACEHOLDER))
#define IS_AHOGHILL_PLAYLIST_PLACEHOLDER_CLASS(klass)                   \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_PLAYLIST_PLACEHOLDER))
#define AHOGHILL_PLAYLIST_PLACEHOLDER_GET_CLASS(obj)                    \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_PLAYLIST_PLACEHOLDER,      \
                               AhoghillPlaylistPlaceholderClass))

typedef struct _AhoghillPlaylistPlaceholderPrivate AhoghillPlaylistPlaceholderPrivate;
typedef struct _AhoghillPlaylistPlaceholder      AhoghillPlaylistPlaceholder;
typedef struct _AhoghillPlaylistPlaceholderClass AhoghillPlaylistPlaceholderClass;

struct _AhoghillPlaylistPlaceholder
{
    NbtkWidget parent;

    AhoghillPlaylistPlaceholderPrivate *priv;
};

struct _AhoghillPlaylistPlaceholderClass
{
    NbtkWidgetClass parent_class;
};

GType ahoghill_playlist_placeholder_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __AHOGHILL_PLAYLIST_PLACEHOLDER_H__ */
