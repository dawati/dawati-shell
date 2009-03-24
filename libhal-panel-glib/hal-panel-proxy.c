#include "hal-panel-proxy.h"
#include "hal-laptop-panel-bindings.h"

G_DEFINE_TYPE (HalPanelProxy, hal_panel_proxy, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAL_PANEL_TYPE_PROXY, HalPanelProxyPrivate))

typedef struct _HalPanelProxyPrivate HalPanelProxyPrivate;

struct _HalPanelProxyPrivate {
  DBusGProxy *proxy;
  DBusGConnection *connection;
};

#define HAL_DBUS_NAME "org.freedesktop.Hal"
#define HAL_DBUS_PANEL_INTERFACE "org.freedesktop.Hal.Device.LaptopPanel"

static void
hal_panel_proxy_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
hal_panel_proxy_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
hal_panel_proxy_dispose (GObject *object)
{
  HalPanelProxyPrivate *priv = GET_PRIVATE (object);

  if (priv->proxy)
  {
    g_object_unref (priv->proxy);
    priv->proxy = NULL;
  }

  if (priv->connection)
  {
    dbus_g_connection_unref (priv->connection);
    priv->connection = NULL;
  }

  G_OBJECT_CLASS (hal_panel_proxy_parent_class)->dispose (object);
}

static void
hal_panel_proxy_finalize (GObject *object)
{
  G_OBJECT_CLASS (hal_panel_proxy_parent_class)->finalize (object);
}

static void
hal_panel_proxy_class_init (HalPanelProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (HalPanelProxyPrivate));

  object_class->get_property = hal_panel_proxy_get_property;
  object_class->set_property = hal_panel_proxy_set_property;
  object_class->dispose = hal_panel_proxy_dispose;
  object_class->finalize = hal_panel_proxy_finalize;
}

static void
hal_panel_proxy_init (HalPanelProxy *self)
{
}

static gboolean
_setup_proxy (HalPanelProxy *proxy,
              const gchar   *device_path)
{
  HalPanelProxyPrivate *priv = GET_PRIVATE (proxy);
  GError *error = NULL;

  priv->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (error)
  {
    g_warning (G_STRLOC ": Unabel to get bus: %s",
               error->message);
    g_clear_error (&error);
    return FALSE;
  }

  priv->proxy = dbus_g_proxy_new_for_name_owner (priv->connection,
                                                 HAL_DBUS_NAME,
                                                 device_path,
                                                 HAL_DBUS_PANEL_INTERFACE,
                                                 &error);

  if (error)
  {
    g_warning (G_STRLOC ": Unable to create proxy for object: %s: %s",
               device_path,
               error->message);
    g_clear_error (&error);
    return FALSE;
  }

  return TRUE;
}

HalPanelProxy *
hal_panel_proxy_new (const gchar *device_path)
{
  HalPanelProxy *proxy;

  proxy = g_object_new (HAL_PANEL_TYPE_PROXY, NULL);

  if (!_setup_proxy (proxy, device_path))
  {
    g_object_unref (proxy);
    return NULL;
  }

  return proxy;
}

typedef struct
{
  HalPanelProxy *proxy;
  GCallback cb;
  gpointer userdata;
  GObject *weak_object;
  DBusGProxyCall *call;
} BrightnessClosure;

static void
_weak_notify_cb (gpointer  data,
                 GObject  *dead_object)
{
  BrightnessClosure *closure = (BrightnessClosure *)data;
  HalPanelProxyPrivate *priv = GET_PRIVATE (closure->proxy);

  dbus_g_proxy_cancel_call (priv->proxy, closure->call);
  g_object_unref (closure->proxy);
  g_slice_free (BrightnessClosure, closure);
}

void
_get_brightness_cb (DBusGProxy *proxy,
                    gint OUT_return_code,
                    GError *error,
                    gpointer userdata)
{
  BrightnessClosure *closure = (BrightnessClosure *)userdata;
  HalPanelProxyGetBrightnessCallback cb;

  cb = (HalPanelProxyGetBrightnessCallback)closure->cb;
  cb (closure->proxy,
      OUT_return_code,
      error,
      closure->weak_object,
      closure->userdata);

  if (closure->weak_object)
  {
    g_object_weak_unref (closure->weak_object, _weak_notify_cb, closure);
  }

  g_object_unref (closure->proxy);
  g_slice_free (BrightnessClosure, closure);
}

void
hal_panel_proxy_get_brightness_async (HalPanelProxy                       *proxy,
                                      HalPanelProxyGetBrightnessCallback   cb,
                                      GObject                             *weak_object,
                                      gpointer                             userdata)
{
  HalPanelProxyPrivate *priv = GET_PRIVATE (proxy);
  DBusGProxyCall *call;
  BrightnessClosure *closure;

  closure = g_slice_new0 (BrightnessClosure);
  closure->proxy = g_object_ref (proxy);
  closure->cb = (GCallback)cb;
  closure->userdata = userdata;
  closure->weak_object = weak_object;

  if (closure->weak_object)
  {
    g_object_weak_ref (closure->weak_object, _weak_notify_cb, closure);
  }

  call = 
    org_freedesktop_Hal_Device_LaptopPanel_get_brightness_async (priv->proxy,
                                                                 _get_brightness_cb,
                                                                 closure);
  closure->call = call;
}

void
_set_brightness_cb (DBusGProxy *proxy,
                    gint OUT_return_code,
                    GError *error,
                    gpointer userdata)
{
  BrightnessClosure *closure = (BrightnessClosure *)userdata;
  HalPanelProxySetBrightnessCallback cb;

  cb = (HalPanelProxySetBrightnessCallback)closure->cb;
  cb (closure->proxy, error, closure->weak_object, closure->userdata);

  if (closure->weak_object)
  {
    g_object_weak_unref (closure->weak_object, _weak_notify_cb, closure);
  }

  g_object_unref (closure->proxy);
  g_slice_free (BrightnessClosure, closure);
}

void
hal_panel_proxy_set_brightness_async (HalPanelProxy                       *proxy,
                                      gint                                 brightness,
                                      HalPanelProxySetBrightnessCallback   cb,
                                      GObject                             *weak_object,
                                      gpointer                             userdata)
{
  HalPanelProxyPrivate *priv = GET_PRIVATE (proxy);
  DBusGProxyCall *call;
  BrightnessClosure *closure;

  closure = g_slice_new0 (BrightnessClosure);
  closure->proxy = g_object_ref (proxy);
  closure->cb = (GCallback)cb;
  closure->userdata = userdata;
  closure->weak_object = weak_object;

  if (closure->weak_object)
  {
    g_object_weak_ref (closure->weak_object, _weak_notify_cb, closure);
  }

  call =
    org_freedesktop_Hal_Device_LaptopPanel_set_brightness_async (priv->proxy,
                                                                 brightness,
                                                                 _set_brightness_cb,
                                                                 closure);
  closure->call = call;
}

