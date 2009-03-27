#ifndef __MNB_CLIPBOARD_STORE_H__
#define __MNB_CLIPBOARD_STORE_H__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MNB_TYPE_CLIPBOARD_STORE                (mnb_clipboard_store_get_type ())
#define MNB_CLIPBOARD_STORE(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_CLIPBOARD_STORE, MnbClipboardStore))
#define MNB_IS_CLIPBOARD_STORE(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_CLIPBOARD_STORE))
#define MNB_CLIPBOARD_STORE_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_CLIPBOARD_STORE, MnbClipboardStoreClass))
#define MNB_IS_CLIPBOARD_STORE_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_CLIPBOARD_STORE))
#define MNB_CLIPBOARD_STORE_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_CLIPBOARD_STORE, MnbClipboardStoreClass))

typedef struct _MnbClipboardStore               MnbClipboardStore;
typedef struct _MnbClipboardStorePrivate        MnbClipboardStorePrivate;
typedef struct _MnbClipboardStoreClass          MnbClipboardStoreClass;

typedef enum {
  MNB_CLIPBOARD_ITEM_INVALID = 0,
  MNB_CLIPBOARD_ITEM_TEXT,
  MNB_CLIPBOARD_ITEM_URIS,
  MNB_CLIPBOARD_ITEM_IMAGE
} MnbClipboardItemType;

struct _MnbClipboardStore
{
  ClutterListModel parent_instance;

  MnbClipboardStorePrivate *priv;
};

struct _MnbClipboardStoreClass
{
  ClutterListModelClass parent_class;
};

GType mnb_clipboard_store_get_type (void) G_GNUC_CONST;

MnbClipboardStore *mnb_clipboard_store_new (void);

G_END_DECLS

#endif /* __MNB_CLIPBOARD_STORE_H__ */
