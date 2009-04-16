#ifndef ANERLEY_ITEM_H
#define ANERLEY_ITEM_H

#include <glib-object.h>

#define ANERLEY_TYPE_ITEM (anerley_item_get_type())

typedef struct
{
  volatile gint refcount;
  gchar *uid;
  GQuark type;
  gpointer data;
  GDestroyNotify data_destroy_notify;
} AnerleyItem;

AnerleyItem *anerley_item_new ();
AnerleyItem *anerley_item_ref (AnerleyItem *item);
void anerley_item_unref (AnerleyItem *item);

GType anerley_item_get_type (void);

#endif /* ANERLEY_ITEM_H */
