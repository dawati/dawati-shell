#ifndef _MUX_SWITCH_BOX
#define _MUX_SWITCH_BOX

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MUX_TYPE_SWITCH_BOX mux_switch_box_get_type()

#define MUX_SWITCH_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MUX_TYPE_SWITCH_BOX, MuxSwitchBox))

#define MUX_SWITCH_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MUX_TYPE_SWITCH_BOX, MuxSwitchBoxClass))

#define MUX_IS_SWITCH_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MUX_TYPE_SWITCH_BOX))

#define MUX_IS_SWITCH_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MUX_TYPE_SWITCH_BOX))

#define MUX_SWITCH_BOX_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MUX_TYPE_SWITCH_BOX, MuxSwitchBoxClass))

typedef struct _MuxSwitchBoxPrivate     MuxSwitchBoxPrivate;

typedef struct {
  GtkHBox parent;

  MuxSwitchBoxPrivate *priv;
} MuxSwitchBox;

typedef struct {
  GtkHBoxClass parent_class;

  void (* switch_toggled) (MuxSwitchBox *switchbox, gboolean state);
} MuxSwitchBoxClass;

GType mux_switch_box_get_type (void);

void mux_switch_box_set_active (MuxSwitchBox *switchbox, gboolean active);

GtkWidget* mux_switch_box_new (const gchar *text);

G_END_DECLS

#endif /* _MUX_SWITCH_BOX */
