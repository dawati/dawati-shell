#include <glib.h>

#include "anerley-item.h"

AnerleyItem *
anerley_item_new ()
{
  AnerleyItem *item;

  item = g_slice_new0 (AnerleyItem);

  return anerley_item_ref (item);
}

AnerleyItem *
anerley_item_ref (AnerleyItem *item)
{
  g_atomic_int_inc (&(item->refcount));
  return item;
}

static void
anerley_item_free (AnerleyItem *item)
{
  if (item->data && item->data_destroy_notify)
  {
    item->data_destroy_notify (item->data);
    item->data = NULL;
  }

  g_free (item->uid);

  g_slice_free (AnerleyItem, item);
}

void
anerley_item_unref (AnerleyItem *item)
{
  if (g_atomic_int_dec_and_test (&(item->refcount)))
  {
    anerley_item_free (item);
  }
}

GType
anerley_item_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
  {
    type = g_boxed_type_register_static ("AnerleyItem",
                                         (GBoxedCopyFunc)anerley_item_ref,
                                         (GBoxedFreeFunc)anerley_item_unref);
  }

  return type;
}
