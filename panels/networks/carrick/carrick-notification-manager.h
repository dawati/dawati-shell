/* carrick-notification-manager.h */

#ifndef _CARRICK_NOTIFICATION_MANAGER_H
#define _CARRICK_NOTIFICATION_MANAGER_H

#include <gtk/gtk.h>
#include <gconnman/gconnman.h>

G_BEGIN_DECLS

#define CARRICK_TYPE_NOTIFICATION_MANAGER carrick_notification_manager_get_type()

#define CARRICK_NOTIFICATION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CARRICK_TYPE_NOTIFICATION_MANAGER, CarrickNotificationManager))

#define CARRICK_NOTIFICATION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CARRICK_TYPE_NOTIFICATION_MANAGER, CarrickNotificationManagerClass))

#define CARRICK_IS_NOTIFICATION_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CARRICK_TYPE_NOTIFICATION_MANAGER))

#define CARRICK_IS_NOTIFICATION_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CARRICK_TYPE_NOTIFICATION_MANAGER))

#define CARRICK_NOTIFICATION_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CARRICK_TYPE_NOTIFICATION_MANAGER, CarrickNotificationManagerClass))

typedef struct _CarrickNotificationManager CarrickNotificationManager;
typedef struct _CarrickNotificationManagerClass CarrickNotificationManagerClass;
typedef struct _CarrickNotificationManagerPrivate CarrickNotificationManagerPrivate;

struct _CarrickNotificationManager
{
  GObject parent;

  CarrickNotificationManagerPrivate *priv;
};

struct _CarrickNotificationManagerClass
{
  GObjectClass parent_class;
};

GType carrick_notification_manager_get_type (void);

CarrickNotificationManager *carrick_notification_manager_new (CmManager *manager);

void carrick_notification_manager_queue_service (CarrickNotificationManager *self, CmService *service, gboolean enabling);

G_END_DECLS

#endif /* _CARRICK_NOTIFICATION_MANAGER_H */
