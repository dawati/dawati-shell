#ifndef __MNB_CLIPBOARD_VIEW_H__
#define __MNB_CLIPBOARD_VIEW_H__

#include <nbtk/nbtk.h>
#include "mnb-clipboard-store.h"

G_BEGIN_DECLS

#define MNB_TYPE_CLIPBOARD_VIEW                 (mnb_clipboard_view_get_type ())
#define MNB_CLIPBOARD_VIEW(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_CLIPBOARD_VIEW, MnbClipboardView))
#define MNB_IS_CLIPBOARD_VIEW(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_CLIPBOARD_VIEW))
#define MNB_CLIPBOARD_VIEW_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_CLIPBOARD_VIEW, MnbClipboardViewClass))
#define MNB_IS_CLIPBOARD_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_CLIPBOARD_VIEW))
#define MNB_CLIPBOARD_VIEW_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_CLIPBOARD_VIEW, MnbClipboardViewClass))

typedef struct _MnbClipboardView                MnbClipboardView;
typedef struct _MnbClipboardViewPrivate         MnbClipboardViewPrivate;
typedef struct _MnbClipboardViewClass           MnbClipboardViewClass;

struct _MnbClipboardView
{
  NbtkWidget parent_instance;

  MnbClipboardViewPrivate *priv;
};

struct _MnbClipboardViewClass
{
  NbtkWidgetClass parent_class;
};

GType mnb_clipboard_view_get_type (void) G_GNUC_CONST;

NbtkWidget *mnb_clipboard_view_new (MnbClipboardStore *store);

G_END_DECLS

#endif /* __MNB_CLIPBOARD_VIEW_H__ */
