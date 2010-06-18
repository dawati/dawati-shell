#ifndef _MEEGO_NETBOOK_NOTIFY_STORE
#define _MEEGO_NETBOOK_NOTIFY_STORE

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define MEEGO_NETBOOK_TYPE_NOTIFY_STORE meego_netbook_notify_store_get_type()

#define MEEGO_NETBOOK_NOTIFY_STORE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEEGO_NETBOOK_TYPE_NOTIFY_STORE, MeegoNetbookNotifyStore))

#define MEEGO_NETBOOK_NOTIFY_STORE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEEGO_NETBOOK_TYPE_NOTIFY_STORE, MeegoNetbookNotifyStoreClass))

#define MEEGO_NETBOOK_IS_NOTIFY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEEGO_NETBOOK_TYPE_NOTIFY_STORE))

#define MEEGO_NETBOOK_IS_NOTIFY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEEGO_NETBOOK_TYPE_NOTIFY_STORE))

#define MEEGO_NETBOOK_NOTIFY_STORE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEEGO_NETBOOK_TYPE_NOTIFY_STORE, MeegoNetbookNotifyStoreClass))

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
} MeegoNetbookNotifyStoreCloseReason;

typedef struct {
  GObject parent;
} MeegoNetbookNotifyStore;

typedef struct {
  GObjectClass parent_class;
  void (*notification_added) (MeegoNetbookNotifyStore *notify, Notification *notification);
  void (*notification_closed) (MeegoNetbookNotifyStore *notify, guint id, MeegoNetbookNotifyStoreCloseReason reason);
} MeegoNetbookNotifyStoreClass;

GType meego_netbook_notify_store_get_type (void);

MeegoNetbookNotifyStore* meego_netbook_notify_store_new (void);

gboolean
meego_netbook_notify_store_close (MeegoNetbookNotifyStore *notify,
				   guint id,
				   MeegoNetbookNotifyStoreCloseReason reason);

void
meego_netbook_notify_store_action (MeegoNetbookNotifyStore    *notify,
				    guint                        id,
				    gchar                       *action);

guint
notification_manager_notify_internal (MeegoNetbookNotifyStore *notify,
                                      guint id,
                                      const gchar *app_name, const gchar *icon,
                                      const gchar *summary, const gchar *body,
                                      const gchar **actions, GHashTable *hints,
                                      gint timeout,
                                      gpointer data);

G_END_DECLS

#endif /* _MEEGO_NETBOOK_NOTIFY_STORE */
