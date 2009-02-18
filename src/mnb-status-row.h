#ifndef __MNB_STATUS_ROW_H__
#define __MNB_STATUS_ROW_H__

#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MNB_TYPE_STATUS_ROW                   (mnb_status_row_get_type ())
#define MNB_STATUS_ROW(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_STATUS_ROW, MnbStatusRow))
#define MNB_IS_STATUS_ROW(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_STATUS_ROW))
#define MNB_STATUS_ROW_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_STATUS_ROW, MnbStatusRowClass))
#define MNB_IS_STATUS_ROW_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_STATUS_ROW))
#define MNB_STATUS_ROW_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_STATUS_ROW, MnbStatusRowClass))

typedef struct _MnbStatusRow          MnbStatusRow;
typedef struct _MnbStatusRowPrivate   MnbStatusRowPrivate;
typedef struct _MnbStatusRowClass     MnbStatusRowClass;

struct _MnbStatusRow
{
  NbtkWidget parent_instance;

  MnbStatusRowPrivate *priv;
};

struct _MnbStatusRowClass
{
  NbtkWidgetClass parent_class;
};

GType mnb_status_row_get_type (void);

NbtkWidget *mnb_status_row_new (const gchar *service_name);

G_END_DECLS

#endif /* __MNB_STATUS_ROW_H__ */
