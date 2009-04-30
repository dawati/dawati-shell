/* carrick-list.h */

#ifndef _CARRICK_LIST_H
#define _CARRICK_LIST_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CARRICK_TYPE_LIST carrick_list_get_type()

#define CARRICK_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CARRICK_TYPE_LIST, CarrickList))

#define CARRICK_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CARRICK_TYPE_LIST, CarrickListClass))

#define CARRICK_IS_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CARRICK_TYPE_LIST))

#define CARRICK_IS_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CARRICK_TYPE_LIST))

#define CARRICK_LIST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CARRICK_TYPE_LIST, CarrickListClass))

typedef struct {
  GtkVBox parent;
} CarrickList;

typedef struct {
  GtkVBoxClass parent_class;
} CarrickListClass;

GType carrick_list_get_type (void);

GtkWidget* carrick_list_new (void);
void carrick_list_add_item (CarrickList *list, GtkWidget *item);

G_END_DECLS

#endif /* _CARRICK_LIST_H */
