#ifndef __GGG_SERVICE_H__
#define __GGG_SERVICE_H__

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define GGG_TYPE_SERVICE (ggg_service_get_type())
#define GGG_SERVICE(obj)                                                \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                GGG_TYPE_SERVICE,                       \
                                GggService))
#define GGG_SERVICE_CLASS(klass)                                        \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             GGG_TYPE_SERVICE,                          \
                             GggServiceClass))
#define IS_GGG_SERVICE(obj)                                             \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                GGG_TYPE_SERVICE))
#define IS_GGG_SERVICE_CLASS(klass)                                     \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             GGG_TYPE_SERVICE))
#define GGG_SERVICE_GET_CLASS(obj)                                      \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               GGG_TYPE_SERVICE,                        \
                               GggServiceClass))

typedef struct _GggServicePrivate GggServicePrivate;
typedef struct _GggService      GggService;
typedef struct _GggServiceClass GggServiceClass;

struct _GggService {
  GObject parent;
  GggServicePrivate *priv;
};

struct _GggServiceClass {
  GObjectClass parent_class;
};

GType ggg_service_get_type (void) G_GNUC_CONST;

GggService * ggg_service_new (DBusGConnection *connection, const char *path);

GggService * ggg_service_new_fake (void);

gboolean ggg_service_is_roaming (GggService *service);

const char * ggg_service_get_mcc (GggService *service);

const char * ggg_service_get_mnc (GggService *service);

void ggg_service_set (GggService *service,
                      const char *apn,
                      const char *username,
                      const char *password);

G_END_DECLS

#endif /* __GGG_SERVICE_H__ */
