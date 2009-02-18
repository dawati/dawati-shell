#ifndef _PENGE_APPS_PANE
#define _PENGE_APPS_PANE

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define PENGE_TYPE_APPS_PANE penge_apps_pane_get_type()

#define PENGE_APPS_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_APPS_PANE, PengeAppsPane))

#define PENGE_APPS_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_APPS_PANE, PengeAppsPaneClass))

#define PENGE_IS_APPS_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_APPS_PANE))

#define PENGE_IS_APPS_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_APPS_PANE))

#define PENGE_APPS_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_APPS_PANE, PengeAppsPaneClass))

typedef struct {
  NbtkTable parent;
} PengeAppsPane;

typedef struct {
  NbtkTableClass parent_class;
} PengeAppsPaneClass;

GType penge_apps_pane_get_type (void);

G_END_DECLS

#endif /* _PENGE_APPS_PANE */

