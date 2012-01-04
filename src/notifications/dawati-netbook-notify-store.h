#ifndef _DAWATI_NETBOOK_NOTIFY_STORE
#define _DAWATI_NETBOOK_NOTIFY_STORE

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define DAWATI_NETBOOK_TYPE_NOTIFY_STORE dawati_netbook_notify_store_get_type()

#define DAWATI_NETBOOK_NOTIFY_STORE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  DAWATI_NETBOOK_TYPE_NOTIFY_STORE, DawatiNetbookNotifyStore))

#define DAWATI_NETBOOK_NOTIFY_STORE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  DAWATI_NETBOOK_TYPE_NOTIFY_STORE, DawatiNetbookNotifyStoreClass))

#define DAWATI_NETBOOK_IS_NOTIFY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  DAWATI_NETBOOK_TYPE_NOTIFY_STORE))

#define DAWATI_NETBOOK_IS_NOTIFY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  DAWATI_NETBOOK_TYPE_NOTIFY_STORE))

#define DAWATI_NETBOOK_NOTIFY_STORE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  DAWATI_NETBOOK_TYPE_NOTIFY_STORE, DawatiNetbookNotifyStoreClass))

typedef struct {
  guint id;
  char *summary;
  char *body;
  char *icon_name;
  gchar *sender;
  gint  timeout_ms;
  GList *actions;

  guint  is_urgent         : 1;
  guint  no_dismiss_button : 1;

  gpointer internal_data;
  GdkPixbuf *icon_pixbuf;
  gint pid;
} Notification;

typedef enum {
  ClosedExpired = 1,
  ClosedDismissed,
  ClosedProgramatically,
  ClosedUnknown,
} DawatiNetbookNotifyStoreCloseReason;

typedef struct {
  GObject parent;
} DawatiNetbookNotifyStore;

typedef struct {
  GObjectClass parent_class;
  void (*action_invoked) (DawatiNetbookNotifyStore *notify, guint id, const gchar *action);
  void (*notification_added) (DawatiNetbookNotifyStore *notify, Notification *notification);
  void (*notification_closed) (DawatiNetbookNotifyStore *notify, guint id, DawatiNetbookNotifyStoreCloseReason reason);
} DawatiNetbookNotifyStoreClass;

GType dawati_netbook_notify_store_get_type (void);

DawatiNetbookNotifyStore* dawati_netbook_notify_store_new (void);

gboolean
dawati_netbook_notify_store_close (DawatiNetbookNotifyStore *notify,
				   guint id,
				   DawatiNetbookNotifyStoreCloseReason reason);

void
dawati_netbook_notify_store_action (DawatiNetbookNotifyStore    *notify,
				    guint                        id,
				    gchar                       *action);

guint
notification_manager_notify_internal (DawatiNetbookNotifyStore *notify,
                                      guint id,
                                      const gchar *app_name, const gchar *icon,
                                      const gchar *summary, const gchar *body,
                                      const gchar **actions, GHashTable *hints,
                                      gint timeout,
                                      gpointer data);

G_END_DECLS

#endif /* _DAWATI_NETBOOK_NOTIFY_STORE */
