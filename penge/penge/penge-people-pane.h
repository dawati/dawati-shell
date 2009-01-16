#ifndef _PENGE_PEOPLE_PANE
#define _PENGE_PEOPLE_PANE

#include <nbtk/nbtk.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define PENGE_TYPE_PEOPLE_PANE penge_people_pane_get_type()

#define PENGE_PEOPLE_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_PEOPLE_PANE, PengePeoplePane))

#define PENGE_PEOPLE_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_PEOPLE_PANE, PengePeoplePaneClass))

#define PENGE_IS_PEOPLE_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_PEOPLE_PANE))

#define PENGE_IS_PEOPLE_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_PEOPLE_PANE))

#define PENGE_PEOPLE_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_PEOPLE_PANE, PengePeoplePaneClass))

typedef struct {
  NbtkTable parent;
} PengePeoplePane;

typedef struct {
  NbtkTableClass parent_class;
} PengePeoplePaneClass;

GType penge_people_pane_get_type (void);

G_END_DECLS

#endif /* _PENGE_PEOPLE_PANE */

