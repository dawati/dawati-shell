#ifndef _PENGE_EVENTS_PANE
#define _PENGE_EVENTS_PANE

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define PENGE_TYPE_EVENTS_PANE penge_events_pane_get_type()

#define PENGE_EVENTS_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_EVENTS_PANE, PengeEventsPane))

#define PENGE_EVENTS_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_EVENTS_PANE, PengeEventsPaneClass))

#define PENGE_IS_EVENTS_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_EVENTS_PANE))

#define PENGE_IS_EVENTS_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_EVENTS_PANE))

#define PENGE_EVENTS_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_EVENTS_PANE, PengeEventsPaneClass))

typedef struct {
  NbtkTable parent;
} PengeEventsPane;

typedef struct {
  NbtkTableClass parent_class;
} PengeEventsPaneClass;

GType penge_events_pane_get_type (void);

G_END_DECLS

#endif /* _PENGE_EVENTS_PANE */

