/* carrick-pane.h */

#ifndef _CARRICK_PANE
#define _CARRICK_PANE

#include <gtk/gtk.h>
#include <gconnman/gconnman.h>
#include "carrick-icon-factory.h"

G_BEGIN_DECLS

#define CARRICK_TYPE_PANE carrick_pane_get_type()

#define CARRICK_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CARRICK_TYPE_PANE, CarrickPane))

#define CARRICK_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CARRICK_TYPE_PANE, CarrickPaneClass))

#define CARRICK_IS_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CARRICK_TYPE_PANE))

#define CARRICK_IS_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CARRICK_TYPE_PANE))

#define CARRICK_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CARRICK_TYPE_PANE, CarrickPaneClass))

typedef struct {
  GtkTable parent;
} CarrickPane;

typedef struct {
  GtkTableClass parent_class;
} CarrickPaneClass;

GType carrick_pane_get_type (void);

GtkWidget* carrick_pane_new (CarrickIconFactory *icon_factory,
                             CmManager          *manager);

G_END_DECLS

#endif /* _CARRICK_PANE */
