#ifndef _MUX_LIGHT_SWITCH
#define _MUX_LIGHT_SWITCH

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MUX_TYPE_LIGHT_SWITCH mux_light_switch_get_type()

#define MUX_LIGHT_SWITCH(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MUX_TYPE_LIGHT_SWITCH, MuxLightSwitch))

#define MUX_LIGHT_SWITCH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MUX_TYPE_LIGHT_SWITCH, MuxLightSwitchClass))

#define MUX_IS_LIGHT_SWITCH(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MUX_TYPE_LIGHT_SWITCH))

#define MUX_IS_LIGHT_SWITCH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MUX_TYPE_LIGHT_SWITCH))

#define MUX_LIGHT_SWITCH_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MUX_TYPE_LIGHT_SWITCH, MuxLightSwitchClass))

typedef struct {
  GtkDrawingArea parent;
} MuxLightSwitch;

typedef struct {
  GtkDrawingAreaClass parent_class;

        void (* switch_flipped) (MuxLightSwitch *lightswitch, gboolean state);
} MuxLightSwitchClass;

GType mux_light_switch_get_type (void);

void mux_light_switch_set_active (MuxLightSwitch *lightswitch, gboolean active);
gboolean mux_light_switch_get_active (MuxLightSwitch *lightswitch);

GtkWidget* mux_light_switch_new (void);

G_END_DECLS

#endif /* _MUX_LIGHT_SWITCH */
