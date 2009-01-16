#ifndef _PENGE_CALENDAR_PANE
#define _PENGE_CALENDAR_PANE

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define PENGE_TYPE_CALENDAR_PANE penge_calendar_pane_get_type()

#define PENGE_CALENDAR_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_CALENDAR_PANE, PengeCalendarPane))

#define PENGE_CALENDAR_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_CALENDAR_PANE, PengeCalendarPaneClass))

#define PENGE_IS_CALENDAR_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_CALENDAR_PANE))

#define PENGE_IS_CALENDAR_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_CALENDAR_PANE))

#define PENGE_CALENDAR_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_CALENDAR_PANE, PengeCalendarPaneClass))

typedef struct {
  NbtkTable parent;
} PengeCalendarPane;

typedef struct {
  NbtkTableClass parent_class;
} PengeCalendarPaneClass;

GType penge_calendar_pane_get_type (void);

G_END_DECLS

#endif /* _PENGE_CALENDAR_PANE */

