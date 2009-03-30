#ifndef _DALSTON_VOLUME_STATUS_ICON
#define _DALSTON_VOLUME_STATUS_ICON

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DALSTON_TYPE_VOLUME_STATUS_ICON dalston_volume_status_icon_get_type()

#define DALSTON_VOLUME_STATUS_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DALSTON_TYPE_VOLUME_STATUS_ICON, DalstonVolumeStatusIcon))

#define DALSTON_VOLUME_STATUS_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DALSTON_TYPE_VOLUME_STATUS_ICON, DalstonVolumeStatusIconClass))

#define DALSTON_IS_VOLUME_STATUS_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DALSTON_TYPE_VOLUME_STATUS_ICON))

#define DALSTON_IS_VOLUME_STATUS_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DALSTON_TYPE_VOLUME_STATUS_ICON))

#define DALSTON_VOLUME_STATUS_ICON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DALSTON_TYPE_VOLUME_STATUS_ICON, DalstonVolumeStatusIconClass))

typedef struct {
  GtkStatusIcon parent;
} DalstonVolumeStatusIcon;

typedef struct {
  GtkStatusIconClass parent_class;
} DalstonVolumeStatusIconClass;

GType dalston_volume_status_icon_get_type (void);

GtkStatusIcon *dalston_volume_status_icon_new (void);

G_END_DECLS

#endif /* _DALSTON_VOLUME_STATUS_ICON */

