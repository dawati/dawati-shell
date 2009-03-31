#ifndef __MNB_CLIPBOARD_ITEM_H__
#define __MNB_CLIPBOARD_ITEM_H__

#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MNB_TYPE_CLIPBOARD_ITEM                 (mnb_clipboard_item_get_type ())
#define MNB_CLIPBOARD_ITEM(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_CLIPBOARD_ITEM, MnbClipboardItem))
#define MNB_IS_CLIPBOARD_ITEM(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_CLIPBOARD_ITEM))
#define MNB_CLIPBOARD_ITEM_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_CLIPBOARD_ITEM, MnbClipboardItemClass))
#define MNB_IS_CLIPBOARD_ITEM_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_CLIPBOARD_ITEM))
#define MNB_CLIPBOARD_ITEM_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_CLIPBOARD_ITEM, MnbClipboardItemClass))

typedef struct _MnbClipboardItem                MnbClipboardItem;
typedef struct _MnbClipboardItemClass           MnbClipboardItemClass;

struct _MnbClipboardItem
{
  NbtkWidget parent_instance;

  ClutterActor *contents;

  ClutterActor *time_label;

  ClutterActor *remove_button;
  ClutterActor *action_button;
};

struct _MnbClipboardItemClass
{
  NbtkWidgetClass parent_class;
};

G_END_DECLS

#endif /* __MNB_CLIPBOARD_ITEM_H__ */
