#ifndef __AHOGHILL_EXAMPLE_TABLE_H__
#define __AHOGHILL_EXAMPLE_TABLE_H__

#include <nbtk/nbtk.h>


G_BEGIN_DECLS

#define AHOGHILL_TYPE_EXAMPLE_TABLE                                     \
   (ahoghill_example_table_get_type())
#define AHOGHILL_EXAMPLE_TABLE(obj)                                     \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                AHOGHILL_TYPE_EXAMPLE_TABLE,            \
                                AhoghillExampleTable))
#define AHOGHILL_EXAMPLE_TABLE_CLASS(klass)                             \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             AHOGHILL_TYPE_EXAMPLE_TABLE,               \
                             AhoghillExampleTableClass))
#define IS_AHOGHILL_EXAMPLE_TABLE(obj)                                  \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                AHOGHILL_TYPE_EXAMPLE_TABLE))
#define IS_AHOGHILL_EXAMPLE_TABLE_CLASS(klass)                          \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             AHOGHILL_TYPE_EXAMPLE_TABLE))
#define AHOGHILL_EXAMPLE_TABLE_GET_CLASS(obj)                           \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               AHOGHILL_TYPE_EXAMPLE_TABLE,             \
                               AhoghillExampleTableClass))

typedef struct _AhoghillExampleTablePrivate AhoghillExampleTablePrivate;
typedef struct _AhoghillExampleTable      AhoghillExampleTable;
typedef struct _AhoghillExampleTableClass AhoghillExampleTableClass;

struct _AhoghillExampleTable
{
    NbtkTable parent;

    AhoghillExampleTablePrivate *priv;
};

struct _AhoghillExampleTableClass
{
    NbtkTableClass parent_class;
};

GType ahoghill_example_table_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __AHOGHILL_EXAMPLE_TABLE_H__ */
