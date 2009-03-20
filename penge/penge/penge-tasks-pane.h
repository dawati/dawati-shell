#ifndef _PENGE_TASKS_PANE
#define _PENGE_TASKS_PANE

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define PENGE_TYPE_TASKS_PANE penge_tasks_pane_get_type()

#define PENGE_TASKS_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_TASKS_PANE, PengeTasksPane))

#define PENGE_TASKS_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_TASKS_PANE, PengeTasksPaneClass))

#define PENGE_IS_TASKS_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_TASKS_PANE))

#define PENGE_IS_TASKS_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_TASKS_PANE))

#define PENGE_TASKS_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_TASKS_PANE, PengeTasksPaneClass))

typedef struct {
  NbtkTable parent;
} PengeTasksPane;

typedef struct {
  NbtkTableClass parent_class;
} PengeTasksPaneClass;

GType penge_tasks_pane_get_type (void);

G_END_DECLS

#endif /* _PENGE_TASKS_PANE */

