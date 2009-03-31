#ifndef _DALSTON_POWER_APPLET
#define _DALSTON_POWER_APPLET

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DALSTON_TYPE_POWER_APPLET dalston_power_applet_get_type()

#define DALSTON_POWER_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DALSTON_TYPE_POWER_APPLET, DalstonPowerApplet))

#define DALSTON_POWER_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DALSTON_TYPE_POWER_APPLET, DalstonPowerAppletClass))

#define DALSTON_IS_POWER_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DALSTON_TYPE_POWER_APPLET))

#define DALSTON_IS_POWER_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DALSTON_TYPE_POWER_APPLET))

#define DALSTON_POWER_APPLET_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DALSTON_TYPE_POWER_APPLET, DalstonPowerAppletClass))

typedef struct {
  GObject parent;
} DalstonPowerApplet;

typedef struct {
  GObjectClass parent_class;
} DalstonPowerAppletClass;

GType dalston_power_applet_get_type (void);

DalstonPowerApplet *dalston_power_applet_new (void);
GtkStatusIcon *dalston_power_applet_get_status_icon (DalstonPowerApplet *applet);
GtkWidget *dalston_power_applet_get_pane (DalstonPowerApplet *applet);

G_END_DECLS

#endif /* _DALSTON_POWER_APPLET */

