#ifndef _DALSTON_VOLUME_PANE
#define _DALSTON_VOLUME_PANE

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DALSTON_TYPE_VOLUME_PANE dalston_volume_pane_get_type()

#define DALSTON_VOLUME_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DALSTON_TYPE_VOLUME_PANE, DalstonVolumePane))

#define DALSTON_VOLUME_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DALSTON_TYPE_VOLUME_PANE, DalstonVolumePaneClass))

#define DALSTON_IS_VOLUME_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DALSTON_TYPE_VOLUME_PANE))

#define DALSTON_IS_VOLUME_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DALSTON_TYPE_VOLUME_PANE))

#define DALSTON_VOLUME_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DALSTON_TYPE_VOLUME_PANE, DalstonVolumePaneClass))

typedef struct {
  GtkHBox parent;
} DalstonVolumePane;

typedef struct {
  GtkHBoxClass parent_class;
} DalstonVolumePaneClass;

GType dalston_volume_pane_get_type (void);

GtkWidget *dalston_volume_pane_new (void);

G_END_DECLS

#endif /* _DALSTON_VOLUME_PANE */

