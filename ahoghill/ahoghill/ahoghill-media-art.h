#ifndef __AHOGHILL_MEDIA_ART_H__
#define __AHOGHILL_MEDIA_ART_H__

#include <nbtk/nbtk-widget.h>

G_BEGIN_DECLS

#define AHOGHILL_TYPE_MEDIA_ART                                         \
   (ahoghill_media_art_get_type())
#define AHOGHILL_MEDIA_ART(obj)                                         \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_MEDIA_ART,                \
                                AhoghillMediaArt))
#define AHOGHILL_MEDIA_ART_CLASS(klass)                                 \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_MEDIA_ART,                   \
                             AhoghillMediaArtClass))
#define IS_AHOGHILL_MEDIA_ART(obj)                                      \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_MEDIA_ART))
#define IS_AHOGHILL_MEDIA_ART_CLASS(klass)                              \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_MEDIA_ART))
#define AHOGHILL_MEDIA_ART_GET_CLASS(obj)                               \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_MEDIA_ART,                 \
                               AhoghillMediaArtClass))

typedef struct _AhoghillMediaArtPrivate AhoghillMediaArtPrivate;
typedef struct _AhoghillMediaArt      AhoghillMediaArt;
typedef struct _AhoghillMediaArtClass AhoghillMediaArtClass;

struct _AhoghillMediaArt
{
    NbtkWidget parent;

    AhoghillMediaArtPrivate *priv;
};

struct _AhoghillMediaArtClass
{
    NbtkWidgetClass parent_class;
};

GType ahoghill_media_art_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __AHOGHILL_MEDIA_ART_H__ */
