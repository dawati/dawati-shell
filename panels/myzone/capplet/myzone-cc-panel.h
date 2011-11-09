#ifndef _MYZONE_CC_PANEL
#define _MYZONE_CC_PANEL

#include <glib-object.h>
#include <libgnome-control-center-extension/cc-panel.h>

G_BEGIN_DECLS

#define MYZONE_TYPE_CC_PANEL myzone_cc_panel_get_type()

#define MYZONE_CC_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MYZONE_TYPE_CC_PANEL, MyzoneCcPanel))

#define MYZONE_CC_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MYZONE_TYPE_CC_PANEL, MyzoneCcPanelClass))

#define MYZONE_IS_CC_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MYZONE_TYPE_CC_PANEL))

#define MYZONE_IS_CC_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MYZONE_TYPE_CC_PANEL))

#define MYZONE_CC_PANEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MYZONE_TYPE_CC_PANEL, MyzoneCcPanelClass))

typedef struct {
  CcPanel parent;
} MyzoneCcPanel;

typedef struct {
  CcPanelClass parent_class;
} MyzoneCcPanelClass;

GType myzone_cc_panel_get_type (void);

G_END_DECLS

#endif /* _MYZONE_CC_PANEL */
