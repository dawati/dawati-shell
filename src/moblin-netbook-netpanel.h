#ifndef _MOBLIN_NETBOOK_NETPANEL_H
#define _MOBLIN_NETBOOK_NETPANEL_H

#include <glib-object.h>
#include <clutter/clutter.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MOBLIN_TYPE_NETBOOK_NETPANEL moblin_netbook_netpanel_get_type()

#define MOBLIN_NETBOOK_NETPANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MOBLIN_TYPE_NETBOOK_NETPANEL, MoblinNetbookNetpanel))

#define MOBLIN_NETBOOK_NETPANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MOBLIN_TYPE_NETBOOK_NETPANEL, MoblinNetbookNetpanelClass))

#define MOBLIN_IS_NETBOOK_NETPANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MOBLIN_TYPE_NETBOOK_NETPANEL))

#define MOBLIN_IS_NETBOOK_NETPANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MOBLIN_TYPE_NETBOOK_NETPANEL))

#define MOBLIN_NETBOOK_NETPANEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MOBLIN_TYPE_NETBOOK_NETPANEL, MoblinNetbookNetpanelClass))

typedef struct _MoblinNetbookNetpanelPrivate MoblinNetbookNetpanelPrivate;

typedef struct {
  NbtkTable parent;
  
  MoblinNetbookNetpanelPrivate *priv;
} MoblinNetbookNetpanel;

typedef struct {
  NbtkTableClass parent_class;

  /* Signals */
  void (* launch)   (MoblinNetbookNetpanel *netpanel, const gchar *url);
  void (* launched) (MoblinNetbookNetpanel *netpanel);
} MoblinNetbookNetpanelClass;

GType moblin_netbook_netpanel_get_type (void);

NbtkWidget* moblin_netbook_netpanel_new (void);

void moblin_netbook_netpanel_focus (MoblinNetbookNetpanel *netpanel);
void moblin_netbook_netpanel_clear (MoblinNetbookNetpanel *netpanel);

G_END_DECLS

#endif /* _MOBLIN_NETBOOK_NETPANEL_H */

