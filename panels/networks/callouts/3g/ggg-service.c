#include <config.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "carrick/connman-service-bindings.h"
#include "ggg-service.h"

#define CONNMAN_SERVICE           "org.moblin.connman"
#define CONNMAN_MANAGER_PATH      "/"
#define CONNMAN_MANAGER_INTERFACE   CONNMAN_SERVICE ".Manager"
#define CONNMAN_SERVICE_INTERFACE CONNMAN_SERVICE ".Service"

struct _GggServicePrivate {
  DBusGProxy *proxy;
  char *name;
  char *mcc, *mnc;
  gboolean roaming;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GGG_TYPE_SERVICE, GggServicePrivate))
G_DEFINE_TYPE (GggService, ggg_service, G_TYPE_OBJECT);

static void
ggg_service_class_init (GggServiceClass *klass)
{
  g_type_class_add_private (klass, sizeof (GggServicePrivate));
}

static void
ggg_service_init (GggService *self)
{
  self->priv = GET_PRIVATE (self);
}

GggService *
ggg_service_new (DBusGConnection *connection, const char *path)
{
  GggService *service;
  DBusGProxy *proxy;
  GHashTable *props;
  GError *error = NULL;
  GValue *value;

  g_assert (connection);
  g_assert (path);

  proxy = dbus_g_proxy_new_for_name (connection,
                                     CONNMAN_SERVICE,
                                     path,
                                     CONNMAN_SERVICE_INTERFACE);

  if (!org_moblin_connman_Service_get_properties (proxy, &props, &error)) {
    g_printerr ("Cannot get properties for service: %s\n", error->message);
    g_error_free (error);
    g_object_unref (proxy);
    return NULL;
  }

  value = g_hash_table_lookup (props, "Type");
  if (!value || !G_VALUE_HOLDS_STRING (value)) {
    g_hash_table_unref (props);
    g_object_unref (proxy);
    return NULL;
  }

  if (g_strcmp0 (g_value_get_string (value), "cellular") != 0) {
    g_hash_table_unref (props);
    g_object_unref (proxy);
    return NULL;
  }

  service = g_object_new (GGG_TYPE_SERVICE, NULL);
  service->priv->proxy = proxy;

  /* TODO: populate from properties */

  return service;
}

GggService *
ggg_service_new_fake (void)
{
  GggService *service;

  service = g_object_new (GGG_TYPE_SERVICE, NULL);
  service->priv->proxy = NULL;

  service->priv->name = g_strdup ("Fake Service");
  service->priv->mcc = g_strdup ("234");
  service->priv->mnc = g_strdup ("15");
  service->priv->roaming = FALSE;

  return service;
}

gboolean
ggg_service_is_roaming (GggService *service)
{
  return service->priv->roaming;
}

const char *
ggg_service_get_mcc (GggService *service)
{
  return service->priv->mcc;
}

const char *
ggg_service_get_mnc (GggService *service)
{
  return service->priv->mnc;
}
