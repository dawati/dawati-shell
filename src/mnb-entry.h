#ifndef __MNB_ENTRY_H__
#define __MNB_ENTRY_H__

#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MNB_TYPE_ENTRY                   (mnb_entry_get_type ())
#define MNB_ENTRY(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_ENTRY, MnbEntry))
#define MNB_IS_ENTRY(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_ENTRY))
#define MNB_ENTRY_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_ENTRY, MnbEntryClass))
#define MNB_IS_ENTRY_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_ENTRY))
#define MNB_ENTRY_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_ENTRY, MnbEntryClass))

typedef struct _MnbEntry          MnbEntry;
typedef struct _MnbEntryPrivate   MnbEntryPrivate;
typedef struct _MnbEntryClass     MnbEntryClass;

struct _MnbEntry
{
  NbtkWidget parent_instance;

  MnbEntryPrivate *priv;
};

struct _MnbEntryClass
{
  NbtkWidgetClass parent_class;

  /* Signals. */
  void (* button_clicked) (MnbEntry *self);
};

GType mnb_entry_get_type (void) G_GNUC_CONST;

NbtkWidget *  mnb_entry_new       (const char   *label);

const gchar * mnb_entry_get_label (MnbEntry     *self);
void          mnb_entry_set_label (MnbEntry     *self,
                                   const gchar  *label);

const gchar * mnb_entry_get_text  (MnbEntry     *self);
void          mnb_entry_set_text  (MnbEntry     *self,
                                   const gchar  *text);

G_END_DECLS

#endif /* __MNB_ENTRY_H__ */
