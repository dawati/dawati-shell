/* carrick-icon-factory.c */

#include "carrick-icon-factory.h"
#include <config.h>

G_DEFINE_TYPE (CarrickIconFactory, carrick_icon_factory, G_TYPE_OBJECT)

#define ICON_FACTORY_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_ICON_FACTORY, CarrickIconFactoryPrivate))

typedef struct _CarrickIconFactoryPrivate CarrickIconFactoryPrivate;

#define PKG_ICON_DIR PKG_DATA_DIR "/" "icons"

static const gchar *icon_names[] = {
  PKG_ICON_DIR "/" "carrick-no-network-normal.png",
  PKG_ICON_DIR "/" "carrick-no-network-active.png",
  PKG_ICON_DIR "/" "carrick-associating-normal.png",
  PKG_ICON_DIR "/" "carrick-associating-active.png",
  PKG_ICON_DIR "/" "carrick-wired-normal.png",
  PKG_ICON_DIR "/" "carrick-wired-active.png",
  PKG_ICON_DIR "/" "carrick-wireless-1-normal.png",
  PKG_ICON_DIR "/" "carrick-wireless-1-active.png",
  PKG_ICON_DIR "/" "carrick-wireless-2-normal.png",
  PKG_ICON_DIR "/" "carrick-wireless-2-active.png",
  PKG_ICON_DIR "/" "carrick-wireless-3-normal.png",
  PKG_ICON_DIR "/" "carrick-wireless-3-active.png"
};

struct _CarrickIconFactoryPrivate
{
  GtkWidget  *no_network_img;
  GtkWidget  *wired_img;
  GtkWidget  *wireless_1_img;
  GtkWidget  *wireless_2_img;
  GtkWidget  *wireless_3_img;
};

static void
carrick_icon_factory_dispose (GObject *object)
{
  G_OBJECT_CLASS (carrick_icon_factory_parent_class)->dispose (object);
}

static void
carrick_icon_factory_class_init (CarrickIconFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CarrickIconFactoryPrivate));

  object_class->dispose = carrick_icon_factory_dispose;
}

static void
carrick_icon_factory_init (CarrickIconFactory *self)
{
  CarrickIconFactoryPrivate *priv = ICON_FACTORY_PRIVATE (self);

  priv->no_network_img = NULL;
  priv->wired_img      = NULL;
  priv->wireless_1_img = NULL;
  priv->wireless_2_img = NULL;
  priv->wireless_3_img = NULL;
}

CarrickIconFactory*
carrick_icon_factory_new (void)
{
  return g_object_new (CARRICK_TYPE_ICON_FACTORY, NULL);
}

CarrickIconState
carrick_icon_factory_state_for_service (CmService *service)
{
  CarrickIconState icon_state = CARRICK_ICON_NO_NETWORK;
  guint  strength;
  const gchar *type;

  if (service)
  {
    strength = cm_service_get_strength (service);
    type = cm_service_get_type (service);
  }
  else
  {
    strength = 0;
    type = NULL;
  }

  if (type)
  {
    if (g_strcmp0 ("ethernet", type) == 0)
    {
      icon_state = CARRICK_ICON_WIRED_NETWORK;
    }
    else if (g_strcmp0 ("wifi", type) == 0)
    {
      if (strength > 70)
        icon_state = CARRICK_ICON_WIRELESS_NETWORK_3;
      else if (strength > 35)
        icon_state = CARRICK_ICON_WIRELESS_NETWORK_2;
      else
        icon_state = CARRICK_ICON_WIRELESS_NETWORK_3;
    }
  }

  return icon_state;
}

const gchar *
carrick_icon_factory_path_for_state (CarrickIconState state)
{
  return icon_names[state];
}

GtkWidget*
carrick_icon_factory_image_for_service (CarrickIconFactory *factory,
                                        CmService          *service)
{
  CarrickIconFactoryPrivate *priv = ICON_FACTORY_PRIVATE (factory);
  CarrickIconState icon_state;
  GtkWidget *icon = NULL;

  icon_state = carrick_icon_factory_state_for_service (service);

  switch (icon_state)
  {
    case CARRICK_ICON_WIRED_NETWORK:
      if (!priv->wired_img)
      {
        priv->wired_img =
          gtk_image_new_from_file (icon_names[CARRICK_ICON_WIRED_NETWORK + 1]);
      }
      icon = priv->wired_img;
      break;
    case CARRICK_ICON_WIRELESS_NETWORK_1:
      if (!priv->wireless_1_img)
      {
        priv->wireless_1_img =
          gtk_image_new_from_file (icon_names[CARRICK_ICON_WIRELESS_NETWORK_1 + 1]);
      }
      icon = priv->wireless_1_img;
      break;
    case CARRICK_ICON_WIRELESS_NETWORK_2:
      if (!priv->wireless_2_img)
      {
        priv->wireless_2_img =
          gtk_image_new_from_file (icon_names[CARRICK_ICON_WIRELESS_NETWORK_2 + 1]);
      }
      icon = priv->wireless_2_img;
      break;
    case CARRICK_ICON_WIRELESS_NETWORK_3:
      if (!priv->wireless_3_img)
      {
        priv->wireless_3_img =
          gtk_image_new_from_file (icon_names[CARRICK_ICON_WIRELESS_NETWORK_3 + 1]);
      }
      icon = priv->wireless_3_img;
      break;
    case CARRICK_ICON_NO_NETWORK:
    default:
      if (!priv->no_network_img)
      {
        priv->no_network_img =
          gtk_image_new_from_file (icon_names[CARRICK_ICON_NO_NETWORK + 1]);
      }
      icon = priv->no_network_img;
      break;
  }

  return icon;
}
