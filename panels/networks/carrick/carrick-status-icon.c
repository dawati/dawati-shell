#include "carrick-status-icon.h"

#include <config.h>
#include <gconnman/gconnman.h>
#include "carrick-icon-factory.h"

G_DEFINE_TYPE (CarrickStatusIcon, carrick_status_icon, GTK_TYPE_STATUS_ICON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_STATUS_ICON, CarrickStatusIconPrivate))

typedef struct _CarrickStatusIconPrivate CarrickStatusIconPrivate;

struct _CarrickStatusIconPrivate {
  CmService          *service;
  CarrickIconFactory *icon_factory;
  CarrickIconState    current_state;
  gboolean            active;
};

enum
{
  PROP_0,
  PROP_ICON_FACTORY,
  PROP_SERVICE
};

static void carrick_status_icon_update_service (CarrickStatusIcon *icon,
                                                CmService         *service);

static void
carrick_status_icon_get_property (GObject *object, guint property_id,
                                  GValue *value, GParamSpec *pspec)
{
  CarrickStatusIconPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_ICON_FACTORY:
      g_value_set_object (value,
                          priv->icon_factory);
      break;
    case PROP_SERVICE:
      g_value_set_object (value,
                          priv->service);
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
  CarrickStatusIconPrivate *priv = GET_PRIVATE (icon);
  CmService *service;

  switch (property_id) {
    case PROP_ICON_FACTORY:
      priv->icon_factory = CARRICK_ICON_FACTORY (g_value_get_object (value));
      break;
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

  pspec = g_param_spec_object ("icon-factory",
                               "Icon factory.",
                               "The icon factory",
                               CARRICK_TYPE_ICON_FACTORY,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_ICON_FACTORY,
                                   pspec);

  pspec = g_param_spec_object ("service",
                               "Service.",
                               "The gconnman CmService to represent.",
                               CM_TYPE_SERVICE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_SERVICE,
                                   pspec);
}

static void
carrick_status_icon_init (CarrickStatusIcon *self)
{
}

static void
carrick_status_icon_update (CarrickStatusIcon *icon)
{
  CarrickStatusIconPrivate *priv = GET_PRIVATE (icon);
  CarrickIconState icon_state = CARRICK_ICON_NO_NETWORK;
  GdkPixbuf *pixbuf;

  icon_state = carrick_icon_factory_get_state_for_service (priv->service);

  if (priv->active)
    icon_state++;

  priv->current_state = icon_state;
  pixbuf = carrick_icon_factory_get_pixbuf_for_state (priv->icon_factory,
                                                      icon_state);
  gtk_status_icon_set_from_pixbuf (GTK_STATUS_ICON (icon),
                                   pixbuf);
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
  }

  carrick_status_icon_update (icon);
}

GtkWidget*
carrick_status_icon_new (CarrickIconFactory *factory,
                         CmService          *service)
{
  return g_object_new (CARRICK_TYPE_STATUS_ICON,
                       "icon-factory",
                       factory,
                       "service",
                       service,
                       NULL);
}
