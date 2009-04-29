/* mnb-toolbar.h */

#ifndef _MNB_TOOLBAR
#define _MNB_TOOLBAR

#include <glib-object.h>
#include <nbtk/nbtk.h>

#include "moblin-netbook.h"
#include "moblin-netbook-tray-manager.h"

G_BEGIN_DECLS

#define MNB_TYPE_TOOLBAR mnb_toolbar_get_type()

#define MNB_TOOLBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_TOOLBAR, MnbToolbar))

#define MNB_TOOLBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_TOOLBAR, MnbToolbarClass))

#define MNB_IS_TOOLBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_TOOLBAR))

#define MNB_IS_TOOLBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_TOOLBAR))

#define MNB_TOOLBAR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_TOOLBAR, MnbToolbarClass))

typedef struct _MnbToolbarPrivate MnbToolbarPrivate;

typedef struct {
  NbtkBin parent;

  MnbToolbarPrivate *priv;
} MnbToolbar;

typedef struct {
  NbtkBinClass parent_class;

  void (*show_completed) (MnbToolbar *toolbar);
  void (*hide_begin)     (MnbToolbar *toolbar);
  void (*hide_completed) (MnbToolbar *toolbar);
} MnbToolbarClass;

GType mnb_toolbar_get_type (void);

NbtkWidget* mnb_toolbar_new (MutterPlugin *plugin);

gboolean mnb_toolbar_is_tray_config_window (MnbToolbar *toolbar, Window xwin);

void mnb_toolbar_append_panel (MnbToolbar  *toolbar,
                               const gchar *name,
                               const gchar *tooltip,
                               const gchar *icon,
                               Window       xid);

void mnb_toolbar_activate_panel (MnbToolbar *toolbar, const gchar *panel_name);
const gchar * mnb_toolbar_get_active_panel_name (MnbToolbar *toolbar);

gboolean mnb_toolbar_in_transition (MnbToolbar *toolbar);

NbtkWidget * mnb_toolbar_get_switcher (MnbToolbar *toolbar);

void mnb_toolbar_set_dont_autohide (MnbToolbar *toolbar, gboolean dont);

void mnb_toolbar_setup_kbd_grabs (MnbToolbar *toolbar);

G_END_DECLS

#endif /* _MNB_TOOLBAR */

