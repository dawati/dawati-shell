#ifndef _HAL_POWER_PROXY
#define _HAL_POWER_PROXY

#include <glib-object.h>

G_BEGIN_DECLS

#define HAL_POWER_TYPE_PROXY hal_power_proxy_get_type()

#define HAL_POWER_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), HAL_POWER_TYPE_PROXY, HalPowerProxy))

#define HAL_POWER_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), HAL_POWER_TYPE_PROXY, HalPowerProxyClass))

#define HAL_POWER_IS_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HAL_POWER_TYPE_PROXY))

#define HAL_POWER_IS_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), HAL_POWER_TYPE_PROXY))

#define HAL_POWER_PROXY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), HAL_POWER_TYPE_PROXY, HalPowerProxyClass))

typedef struct {
  GObject parent;
} HalPowerProxy;

typedef struct {
  GObjectClass parent_class;
} HalPowerProxyClass;

GType hal_power_proxy_get_type (void);

HalPowerProxy *hal_power_proxy_new (void);
typedef void (*HalPowerProxyCallback) (HalPowerProxy *proxy,
                                       const GError  *error,
                                       GObject       *weak_object,
                                       gpointer       userdata);

void hal_power_proxy_shutdown (HalPowerProxy         *proxy,
                               HalPowerProxyCallback  cb,
                               GObject               *weak_object,
                               gpointer               userdata);

void hal_power_proxy_suspend (HalPowerProxy         *proxy,
                              HalPowerProxyCallback  cb,
                              GObject               *weak_object,
                              gpointer               userdata);


G_END_DECLS

#endif /* _HAL_POWER_PROXY */
