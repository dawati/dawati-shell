/* carrick-applet.h */

#ifndef _CARRICK_APPLET
#define _CARRICK_APPLET

#include <gtk/gtk.h>
#include <gconnman/gconnman.h>

G_BEGIN_DECLS

#define CARRICK_TYPE_APPLET carrick_applet_get_type()

#define CARRICK_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CARRICK_TYPE_APPLET, CarrickApplet))

#define CARRICK_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CARRICK_TYPE_APPLET, CarrickAppletClass))

#define CARRICK_IS_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CARRICK_TYPE_APPLET))

#define CARRICK_IS_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CARRICK_TYPE_APPLET))

#define CARRICK_APPLET_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CARRICK_TYPE_APPLET, CarrickAppletClass))

typedef struct {
  GObject parent;
} CarrickApplet;

typedef struct {
  GObjectClass parent_class;
} CarrickAppletClass;

GType carrick_applet_get_type (void);

CarrickApplet* carrick_applet_new (void);
GtkWidget* carrick_applet_get_pane (CarrickApplet *applet);
GtkWidget* carrick_applet_get_icon (CarrickApplet *applet);

G_END_DECLS

#endif /* _CARRICK_APPLET */
