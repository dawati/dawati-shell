#ifndef _DALSTON_VOLUME_APPLET
#define _DALSTON_VOLUME_APPLET

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DALSTON_TYPE_VOLUME_APPLET dalston_volume_applet_get_type()

#define DALSTON_VOLUME_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DALSTON_TYPE_VOLUME_APPLET, DalstonVolumeApplet))

#define DALSTON_VOLUME_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DALSTON_TYPE_VOLUME_APPLET, DalstonVolumeAppletClass))

#define DALSTON_IS_VOLUME_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DALSTON_TYPE_VOLUME_APPLET))

#define DALSTON_IS_VOLUME_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DALSTON_TYPE_VOLUME_APPLET))

#define DALSTON_VOLUME_APPLET_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DALSTON_TYPE_VOLUME_APPLET, DalstonVolumeAppletClass))

typedef struct {
  GObject parent;
} DalstonVolumeApplet;

typedef struct {
  GObjectClass parent_class;
} DalstonVolumeAppletClass;

GType dalston_volume_applet_get_type (void);

DalstonVolumeApplet *dalston_volume_applet_new (void);
GtkStatusIcon *dalston_volume_applet_get_status_icon (DalstonVolumeApplet *applet);
GtkWidget *dalston_volume_applet_get_pane (DalstonVolumeApplet *applet);

G_END_DECLS

#endif /* _DALSTON_VOLUME_APPLET */


