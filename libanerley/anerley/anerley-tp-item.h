#ifndef _ANERLEY_TP_ITEM
#define _ANERLEY_TP_ITEM

#include <glib-object.h>
#include <anerley/anerley-item.h>
#include <telepathy-glib/contact.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_TP_ITEM anerley_tp_item_get_type()

#define ANERLEY_TP_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANERLEY_TYPE_TP_ITEM, AnerleyTpItem))

#define ANERLEY_TP_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), ANERLEY_TYPE_TP_ITEM, AnerleyTpItemClass))

#define ANERLEY_IS_TP_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANERLEY_TYPE_TP_ITEM))

#define ANERLEY_IS_TP_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANERLEY_TYPE_TP_ITEM))

#define ANERLEY_TP_ITEM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANERLEY_TYPE_TP_ITEM, AnerleyTpItemClass))

typedef struct {
  AnerleyItem parent;
} AnerleyTpItem;

typedef struct {
  AnerleyItemClass parent_class;
} AnerleyTpItemClass;

GType anerley_tp_item_get_type (void);

AnerleyTpItem *anerley_tp_item_new (TpContact *contact);

G_END_DECLS

#endif /* _ANERLEY_TP_ITEM */


