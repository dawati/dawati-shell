#ifndef _MOBLIN_NETBOOK_NOTIFY_STORE
#define _MOBLIN_NETBOOK_NOTIFY_STORE

#include <glib-object.h>

G_BEGIN_DECLS

#define MOBLIN_NETBOOK_TYPE_NOTIFY_STORE moblin_netbook_notify_store_get_type()

#define MOBLIN_NETBOOK_NOTIFY_STORE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MOBLIN_NETBOOK_TYPE_NOTIFY_STORE, MoblinNetbookNotifyStore))

#define MOBLIN_NETBOOK_NOTIFY_STORE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MOBLIN_NETBOOK_TYPE_NOTIFY_STORE, MoblinNetbookNotifyStoreClass))

#define MOBLIN_NETBOOK_IS_NOTIFY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MOBLIN_NETBOOK_TYPE_NOTIFY_STORE))

#define MOBLIN_NETBOOK_IS_NOTIFY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MOBLIN_NETBOOK_TYPE_NOTIFY_STORE))

#define MOBLIN_NETBOOK_NOTIFY_STORE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MOBLIN_NETBOOK_TYPE_NOTIFY_STORE, MoblinNetbookNotifyStoreClass))

typedef struct {
  guint id;
  char *summary;
  char *body;
  char *icon_name;
  gchar *sender;
  guint timeout_id;
  GHashTable *actions;
  gboolean    is_urgent; 

} Notification;

typedef enum {
  ClosedExpired = 1,
  ClosedDismissed,
  ClosedProgramatically,
  ClosedUnknown,
} MoblinNetbookNotifyStoreCloseReason;

typedef struct {
  GObject parent;
} MoblinNetbookNotifyStore;

typedef struct {
  GObjectClass parent_class;  
  void (*notification_added) (MoblinNetbookNotifyStore *notify, Notification *notification);
  void (*notification_closed) (MoblinNetbookNotifyStore *notify, guint id, MoblinNetbookNotifyStoreCloseReason reason);
} MoblinNetbookNotifyStoreClass;

GType moblin_netbook_notify_store_get_type (void);

MoblinNetbookNotifyStore* moblin_netbook_notify_store_new (void);

gboolean 
moblin_netbook_notify_store_close (MoblinNetbookNotifyStore *notify, 
				   guint id, 
				   MoblinNetbookNotifyStoreCloseReason reason);

void
moblin_netbook_notify_store_action (MoblinNetbookNotifyStore    *notify, 
				    guint                        id,
				    gchar                       *action);

G_END_DECLS

#endif /* _MOBLIN_NETBOOK_NOTIFY_STORE */
