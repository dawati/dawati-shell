#ifndef _DALSTON_BRIGHTNESS_MANAGER
#define _DALSTON_BRIGHTNESS_MANAGER

#include <glib-object.h>

G_BEGIN_DECLS

#define DALSTON_TYPE_BRIGHTNESS_MANAGER dalston_brightness_manager_get_type()

#define DALSTON_BRIGHTNESS_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DALSTON_TYPE_BRIGHTNESS_MANAGER, DalstonBrightnessManager))

#define DALSTON_BRIGHTNESS_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DALSTON_TYPE_BRIGHTNESS_MANAGER, DalstonBrightnessManagerClass))

#define DALSTON_IS_BRIGHTNESS_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DALSTON_TYPE_BRIGHTNESS_MANAGER))

#define DALSTON_IS_BRIGHTNESS_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DALSTON_TYPE_BRIGHTNESS_MANAGER))

#define DALSTON_BRIGHTNESS_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DALSTON_TYPE_BRIGHTNESS_MANAGER, DalstonBrightnessManagerClass))

typedef struct {
  GObject parent;
} DalstonBrightnessManager;

typedef struct {
  GObjectClass parent_class;
  void (*num_levels_changed) (DalstonBrightnessManager *manager, gint num_levels);
  void (*brightness_changed) (DalstonBrightnessManager *manager, gint new_value);
} DalstonBrightnessManagerClass;

GType dalston_brightness_manager_get_type (void);

DalstonBrightnessManager *dalston_brightness_manager_new (void);

void dalston_brightness_manager_start_monitoring (DalstonBrightnessManager *manager);
void dalston_brightness_manager_stop_monitoring (DalstonBrightnessManager *manager);
void dalston_brightness_manager_set_brightness (DalstonBrightnessManager *manager,
                                                gint                      value);
gboolean dalston_brightness_manager_is_controllable (DalstonBrightnessManager *manager);

G_END_DECLS

#endif /* _DALSTON_BRIGHTNESS_MANAGER */

