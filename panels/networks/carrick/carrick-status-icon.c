#include "carrick-status-icon.h"

#include <config.h>
#include <gconnman/gconnman.h>

G_DEFINE_TYPE (CarrickStatusIcon, carrick_status_icon, GTK_TYPE_STATUS_ICON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_STATUS_ICON, CarrickStatusIconPrivate))

typedef struct _CarrickStatusIconPrivate CarrickStatusIconPrivate;

typedef enum {
  CARRICK_ICON_NO_NETWORK,
  CARRICK_ICON_NO_NETWORK_ACTIVE,
  CARRICK_ICON_WIRED_NETWORK,
  CARRICK_ICON_WIRED_NETWORK_ACTIVE,
  CARRICK_ICON_WIRELESS_NETWORK_1,
  CARRICK_ICON_WIRELESS_NETWORK_1_ACTIVE,
  CARRICK_ICON_WIRELESS_NETWORK_2,
  CARRICK_ICON_WIRELESS_NETWORK_2_ACTIVE,
  CARRICK_ICON_WIRELESS_NETWORK_3,
  CARRICK_ICON_WIRELESS_NETWORK_3_ACTIVE
} CarrickIconState;

struct _CarrickStatusIconPrivate {
  CmService       *service;
  CarrickIconState current_state;
  gboolean         active;
};

enum
{
  PROP_0,
  PROP_SERVICE
};

static void carrick_status_icon_update_service (CarrickStatusIcon *icon,
                                                CmService         *service);

#define PKG_ICON_DIR PKG_DATA_DIR "/" "icons"

static const gchar *icon_names[] = {
  PKG_ICON_DIR "/" "carrick-no-network-normal.png",
  PKG_ICON_DIR "/" "carrick-no-network-active.png",
  PKG_ICON_DIR "/" "carrick-wired-normal.png",
  PKG_ICON_DIR "/" "carrick-wired-active.png",
  PKG_ICON_DIR "/" "carrick-wireless-1-normal.png",
  PKG_ICON_DIR "/" "carrick-wireless-1-active.png",
  PKG_ICON_DIR "/" "carrick-wireless-2-normal.png",
  PKG_ICON_DIR "/" "carrick-wireless-2-active.png",
  PKG_ICON_DIR "/" "carrick-wireless-3-normal.png",
  PKG_ICON_DIR "/" "carrick-wireless-3-active.png"
};

static void
carrick_status_icon_get_property (GObject *object, guint property_id,
                                  GValue *value, GParamSpec *pspec)
{
  CarrickStatusIconPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_SERVICE:
      g_value_set_object (value, priv->service);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
carrick_status_icon_set_property (GObject *object, guint property_id,
                                  const GValue *value, GParamSpec *pspec)
{
  CarrickStatusIcon *icon = (CarrickStatusIcon *)object;
  CmService *service;

  switch (property_id) {
    case PROP_SERVICE:
      service = CM_SERVICE (g_value_get_object (value));
      carrick_status_icon_update_service (icon,
                                          service);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
carrick_status_icon_dispose (GObject *object)
{
  carrick_status_icon_update_service (CARRICK_STATUS_ICON (object),
                                      NULL);

  G_OBJECT_CLASS (carrick_status_icon_parent_class)->dispose (object);
}

static void
carrick_status_icon_finalize (GObject *object)
{
  G_OBJECT_CLASS (carrick_status_icon_parent_class)->finalize (object);
}

static void
carrick_status_icon_class_init (CarrickStatusIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (CarrickStatusIconPrivate));

  object_class->get_property = carrick_status_icon_get_property;
  object_class->set_property = carrick_status_icon_set_property;
  object_class->dispose = carrick_status_icon_dispose;
  object_class->finalize = carrick_status_icon_finalize;

  pspec = g_param_spec_object ("service",
                               "Service.",
                               "The gconnman CmService to represent.",
                               CM_TYPE_SERVICE,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SERVICE, pspec);
}

static void
carrick_status_icon_init (CarrickStatusIcon *self)
{
}

static void
carrick_status_icon_update (CarrickStatusIcon *icon)
{
  CarrickStatusIconPrivate *priv = GET_PRIVATE (icon);
  CarrickIconState icon_state;
  const gchar *state, *type;

  state = cm_service_get_state (priv->service);
  type = cm_service_get_type (priv->service);

  if (!state && !type)
  {
    icon_state = CARRICK_ICON_NO_NETWORK;
  }
  else if (g_strcmp0 ("ready", state))
  {
    icon_state = CARRICK_ICON_NO_NETWORK;
  }
  else
  {
    if (!g_strcmp0 ("ethernet", type))
      icon_state = CARRICK_ICON_WIRED_NETWORK;
    if (!g_strcmp0 ("wifi", type))
    {
      if (!g_strcmp0 ("wifi_low", state))
        icon_state = CARRICK_ICON_WIRELESS_NETWORK_1;
      if (!g_strcmp0 ("wifi_med", state))
        icon_state = CARRICK_ICON_WIRELESS_NETWORK_2;
      if (!g_strcmp0 ("wifi_hight", state))
        icon_state = CARRICK_ICON_WIRELESS_NETWORK_3;
    }
  }

  if (priv->active)
    icon_state++;

  gtk_status_icon_set_from_file (GTK_STATUS_ICON (icon),
                                   icon_names[icon_state]);
  priv->current_state = icon_state;
}

void
carrick_status_icon_set_active (CarrickStatusIcon *icon,
                                gboolean           active)
{
  CarrickStatusIconPrivate *priv = GET_PRIVATE (icon);

  priv->active = active;
  carrick_status_icon_update (icon);
}

static void
carrick_status_icon_update_service (CarrickStatusIcon *icon,
                                    CmService         *service)
{
  CarrickStatusIconPrivate *priv = GET_PRIVATE (icon);

  if (priv->service)
  {
    g_object_unref (priv->service);
    priv->service = NULL;
  }

  if (service)
  {
    priv->service = g_object_ref (service);

    carrick_status_icon_update (icon);
  }
}

GtkWidget*
carrick_status_icon_new (CmService *service)
{
  return g_object_new (CARRICK_TYPE_STATUS_ICON,
                                  "service",
                                  service,
                                  NULL);
}

const gchar *
carrick_status_icon_get_icon_path (CarrickStatusIcon *icon)
{
  CarrickStatusIconPrivate *priv = GET_PRIVATE (icon);

  return g_strdup (icon_names[priv->current_state]);
}

const gchar *
carrick_status_icon_path_for_state (CmService *service)
{
  CarrickIconState icon_state;
  const gchar *state, *type;

  state = cm_service_get_state (service);
  type = cm_service_get_type (service);

  if (!state && !type)
  {
    icon_state = CARRICK_ICON_NO_NETWORK;
  }
  else if (g_strcmp0 ("ready", state))
  {
    icon_state = CARRICK_ICON_NO_NETWORK;
  }
  else
  {
    if (!g_strcmp0 ("ethernet", type))
      icon_state = CARRICK_ICON_WIRED_NETWORK;
    if (!g_strcmp0 ("wifi", type))
    {
      if (!g_strcmp0 ("wifi_low", state))
        icon_state = CARRICK_ICON_WIRELESS_NETWORK_1;
      if (!g_strcmp0 ("wifi_med", state))
        icon_state = CARRICK_ICON_WIRELESS_NETWORK_2;
      if (!g_strcmp0 ("wifi_hight", state))
        icon_state = CARRICK_ICON_WIRELESS_NETWORK_3;
    }
  }
  return icon_names[icon_state];
}
