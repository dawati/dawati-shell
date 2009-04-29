#ifndef __NBTK_FIXED_H__
#define __NBTK_FIXED_H__

#include <nbtk/nbtk.h>


G_BEGIN_DECLS

#define NBTK_TYPE_FIXED                                                 \
   (nbtk_fixed_get_type())
#define NBTK_FIXED(obj)                                                 \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                NBTK_TYPE_FIXED,                        \
                                NbtkFixed))
#define NBTK_FIXED_CLASS(klass)                                         \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             NBTK_TYPE_FIXED,                           \
                             NbtkFixedClass))
#define IS_NBTK_FIXED(obj)                                              \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                NBTK_TYPE_FIXED))
#define IS_NBTK_FIXED_CLASS(klass)                                      \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             NBTK_TYPE_FIXED))
#define NBTK_FIXED_GET_CLASS(obj)                                       \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               NBTK_TYPE_FIXED,                         \
                               NbtkFixedClass))

typedef struct _NbtkFixedPrivate NbtkFixedPrivate;
typedef struct _NbtkFixed      NbtkFixed;
typedef struct _NbtkFixedClass NbtkFixedClass;

struct _NbtkFixed
{
    NbtkWidget parent;

    NbtkFixedPrivate *priv;
};

struct _NbtkFixedClass
{
    NbtkWidgetClass parent_class;
};

GType nbtk_fixed_get_type (void) G_GNUC_CONST;
void nbtk_fixed_add_actor (NbtkFixed    *fixed,
                           ClutterActor *actor);

G_END_DECLS

#endif /* __NBTK_FIXED_H__ */
