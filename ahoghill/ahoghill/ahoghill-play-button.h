#ifndef __AHOGHILL_PLAY_BUTTON_H__
#define __AHOGHILL_PLAY_BUTTON_H__

#include <nbtk/nbtk.h>


G_BEGIN_DECLS

#define AHOGHILL_TYPE_PLAY_BUTTON                                       \
   (ahoghill_play_button_get_type())
#define AHOGHILL_PLAY_BUTTON(obj)                                       \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_PLAY_BUTTON,              \
                                AhoghillPlayButton))
#define AHOGHILL_PLAY_BUTTON_CLASS(klass)                               \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_PLAY_BUTTON,                 \
                             AhoghillPlayButtonClass))
#define IS_AHOGHILL_PLAY_BUTTON(obj)                                    \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_PLAY_BUTTON))
#define IS_AHOGHILL_PLAY_BUTTON_CLASS(klass)                            \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_PLAY_BUTTON))
#define AHOGHILL_PLAY_BUTTON_GET_CLASS(obj)                             \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_PLAY_BUTTON,               \
                               AhoghillPlayButtonClass))

typedef struct _AhoghillPlayButtonPrivate AhoghillPlayButtonPrivate;
typedef struct _AhoghillPlayButton      AhoghillPlayButton;
typedef struct _AhoghillPlayButtonClass AhoghillPlayButtonClass;

struct _AhoghillPlayButton
{
    NbtkButton parent;

    AhoghillPlayButtonPrivate *priv;
};

struct _AhoghillPlayButtonClass
{
    NbtkButtonClass parent_class;
};

GType ahoghill_play_button_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __AHOGHILL_PLAY_BUTTON_H__ */
