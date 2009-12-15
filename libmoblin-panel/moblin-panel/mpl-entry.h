#ifndef __MPL_ENTRY_H__
#define __MPL_ENTRY_H__

#include <mx/mx.h>

G_BEGIN_DECLS

#define MPL_TYPE_ENTRY                   (mpl_entry_get_type ())
#define MPL_ENTRY(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPL_TYPE_ENTRY, MplEntry))
#define MPL_IS_ENTRY(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPL_TYPE_ENTRY))
#define MPL_ENTRY_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), MPL_TYPE_ENTRY, MplEntryClass))
#define MPL_IS_ENTRY_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), MPL_TYPE_ENTRY))
#define MPL_ENTRY_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), MPL_TYPE_ENTRY, MplEntryClass))

typedef struct _MplEntry          MplEntry;
typedef struct _MplEntryPrivate   MplEntryPrivate;
typedef struct _MplEntryClass     MplEntryClass;

struct _MplEntry
{
  MxFrame parent_instance;

  MplEntryPrivate *priv;
};

struct _MplEntryClass
{
  MxFrameClass parent_class;

  /* Signals. */
  void (* button_clicked) (MplEntry *self);
  void (* text_changed)   (MplEntry *self);
  void (* keynav_event)   (MplEntry *self);
};

GType mpl_entry_get_type (void) G_GNUC_CONST;

MxWidget *  mpl_entry_new       (const char   *label);

const gchar * mpl_entry_get_label (MplEntry     *self);
void          mpl_entry_set_label (MplEntry     *self,
                                   const gchar  *label);

const gchar * mpl_entry_get_text  (MplEntry     *self);
void          mpl_entry_set_text  (MplEntry     *self,
                                   const gchar  *text);

MxWidget * mpl_entry_get_mx_entry  (MplEntry     *self);

G_END_DECLS

#endif /* __MPL_ENTRY_H__ */
