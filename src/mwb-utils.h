
#ifndef _MWB_UTILS_H
#define _MWB_UTILS_H

#include <glib-object.h>
#include <clutter/clutter.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MWB_PIXBOUND(u) CLUTTER_UNITS_FROM_DEVICE(CLUTTER_UNITS_TO_DEVICE(u))

gboolean
mwb_utils_focus_on_click_cb (ClutterActor       *actor,
                             ClutterButtonEvent *event,
                             gpointer            swallow_event);

NbtkWidget *
mwb_utils_button_new ();

G_END_DECLS

#endif /* _MWB_UTILS_H */

