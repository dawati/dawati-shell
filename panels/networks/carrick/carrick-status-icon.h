#ifndef _CARRICK_STATUS_ICON
#define _CARRICK_STATUS_ICON

#include <gtk/gtk.h>
#include <gconnman/gconnman.h>

G_BEGIN_DECLS

#define CARRICK_TYPE_STATUS_ICON carrick_status_icon_get_type()

#define CARRICK_STATUS_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CARRICK_TYPE_STATUS_ICON, CarrickStatusIcon))

#define CARRICK_STATUS_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CARRICK_TYPE_STATUS_ICON, CarrickStatusIconClass))

#define CARRICK_IS_STATUS_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CARRICK_TYPE_STATUS_ICON))

#define CARRICK_IS_STATUS_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CARRICK_TYPE_STATUS_ICON))

#define CARRICK_STATUS_ICON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CARRICK_TYPE_STATUS_ICON, CarrickStatusIconClass))

typedef struct {
  GtkStatusIcon parent;
} CarrickStatusIcon;

typedef struct {
  GtkStatusIconClass parent_class;
} CarrickStatusIconClass;

GType carrick_status_icon_get_type (void);

GtkWidget* carrick_status_icon_new (CmService *service);

void carrick_status_icon_set_active (CarrickStatusIcon *icon,
                                     gboolean           active);
G_END_DECLS

#endif /* _CARRICK_STATUS_ICON */
