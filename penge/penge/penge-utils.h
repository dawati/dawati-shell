#include <glib.h>
#include <clutter/clutter.h>

gchar *penge_utils_format_time (GTimeVal *time_);
gchar *penge_utils_get_thumbnail_path (const gchar *uri);
void penge_utils_load_stylesheet (void);
void penge_utils_signal_activated (ClutterActor *actor);
