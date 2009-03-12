#ifndef __MNB_STATUS_ENTRY_H__
#define __MNB_STATUS_ENTRY_H__

#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MNB_TYPE_STATUS_ENTRY                   (mnb_status_entry_get_type ())
#define MNB_STATUS_ENTRY(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_STATUS_ENTRY, MnbStatusEntry))
#define MNB_IS_STATUS_ENTRY(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_STATUS_ENTRY))
#define MNB_STATUS_ENTRY_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_STATUS_ENTRY, MnbStatusEntryClass))
#define MNB_IS_STATUS_ENTRY_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_STATUS_ENTRY))
#define MNB_STATUS_ENTRY_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_STATUS_ENTRY, MnbStatusEntryClass))

typedef struct _MnbStatusEntry          MnbStatusEntry;
typedef struct _MnbStatusEntryPrivate   MnbStatusEntryPrivate;
typedef struct _MnbStatusEntryClass     MnbStatusEntryClass;

struct _MnbStatusEntry
{
  NbtkWidget parent_instance;

  MnbStatusEntryPrivate *priv;
};

struct _MnbStatusEntryClass
{
  NbtkWidgetClass parent_class;

  void (* status_changed) (MnbStatusEntry *entry,
                           const gchar    *new_status_text);
  void (* update_cancelled) (MnbStatusEntry *entry);
};

GType mnb_status_entry_get_type (void);

NbtkWidget *mnb_status_entry_new (const gchar *service_name);

void mnb_status_entry_show_button (MnbStatusEntry *entry,
                                   gboolean        show);

gboolean mnb_status_entry_get_is_active (MnbStatusEntry *entry);
void     mnb_status_entry_set_is_active (MnbStatusEntry *entry,
                                         gboolean        is_active);
gboolean mnb_status_entry_get_in_hover  (MnbStatusEntry *entry);
void     mnb_status_entry_set_in_hover  (MnbStatusEntry *entry,
                                         gboolean        in_hover);

void                  mnb_status_entry_set_status_text (MnbStatusEntry *entry,
                                                        const gchar    *status_text,
                                                        GTimeVal       *status_time);
G_CONST_RETURN gchar *mnb_status_entry_get_status_text (MnbStatusEntry *entry);

G_END_DECLS

#endif /* __MNB_STATUS_ENTRY_H__ */
