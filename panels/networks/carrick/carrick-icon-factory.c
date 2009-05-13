/* carrick-icon-factory.c */

#include "carrick-icon-factory.h"
#include <config.h>

G_DEFINE_TYPE (CarrickIconFactory, carrick_icon_factory, G_TYPE_OBJECT)

#define ICON_FACTORY_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_ICON_FACTORY, CarrickIconFactoryPrivate))

typedef struct _CarrickIconFactoryPrivate CarrickIconFactoryPrivate;

#define PKG_ICON_DIR PKG_DATA_DIR "/" "icons"

static const gchar *icon_names[] = {
  PKG_ICON_DIR "/" "network-active.png",
  PKG_ICON_DIR "/" "network-active-hover.png",
  PKG_ICON_DIR "/" "network-connecting.png",
  PKG_ICON_DIR "/" "network-connecting-hover.png",
  PKG_ICON_DIR "/" "network-error.png",
  PKG_ICON_DIR "/" "network-error-hover.png",
  PKG_ICON_DIR "/" "network-offline.png",
  PKG_ICON_DIR "/" "network-offline-hover.png",
  PKG_ICON_DIR "/" "network-wireless-signal-weak.png",
  PKG_ICON_DIR "/" "network-wireless-signal-weak-hover.png",
  PKG_ICON_DIR "/" "network-wireless-signal-good.png",
  PKG_ICON_DIR "/" "network-wireless-signal-good-hover.png",
  PKG_ICON_DIR "/" "network-wireless-signal-strong.png",
  PKG_ICON_DIR "/" "network-wireless-signal-strong-hover.png"
};

struct _CarrickIconFactoryPrivate
{
  GdkPixbuf *active_img;
  GdkPixbuf *active_hov_img;
  GdkPixbuf *connecting_img;
  GdkPixbuf *connecting_hov_img;
  GdkPixbuf *error_img;
  GdkPixbuf *error_hov_img;
  GdkPixbuf *offline_img;
  GdkPixbuf *offline_hov_img;
  GdkPixbuf *wireless_weak_img;
  GdkPixbuf *wireless_weak_hov_img;
  GdkPixbuf *wireless_good_img;
  GdkPixbuf *wireless_good_hov_img;
  GdkPixbuf *wireless_strong_img;
  GdkPixbuf *wireless_strong_hov_img;
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

  priv->active_img = NULL;
  priv->active_hov_img = NULL;
  priv->connecting_img = NULL;
  priv->connecting_hov_img = NULL;
  priv->error_img = NULL;
  priv->error_hov_img = NULL;
  priv->offline_img = NULL;
  priv->offline_hov_img = NULL;
  priv->wireless_weak_img = NULL;
  priv->wireless_weak_hov_img = NULL;
  priv->wireless_good_img = NULL;
  priv->wireless_good_hov_img = NULL;
  priv->wireless_strong_img = NULL;
  priv->wireless_strong_hov_img = NULL;
}

CarrickIconFactory*
carrick_icon_factory_new (void)
{
  return g_object_new (CARRICK_TYPE_ICON_FACTORY, NULL);
}

CarrickIconState
carrick_icon_factory_get_state_for_service (CmService *service)
{
  CarrickIconState icon_state = ICON_OFFLINE;
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
      icon_state = ICON_ACTIVE;
    }
    else if (g_strcmp0 ("wifi", type) == 0)
    {
      if (strength > 70)
        icon_state = ICON_WIRELESS_NETWORK_STRONG;
      else if (strength > 35)
        icon_state = ICON_WIRELESS_NETWORK_GOOD;
      else
        icon_state = ICON_WIRELESS_NETWORK_WEAK;
    }
  }

  return icon_state;
}

const gchar *
carrick_icon_factory_get_path_for_state (CarrickIconState state)
{
  return icon_names[state];
}

const gchar *
carrick_icon_factory_get_path_for_service (CmService *service)
{
  CarrickIconState state = carrick_icon_factory_get_state_for_service (service);
  return carrick_icon_factory_get_path_for_state (state);
}

GdkPixbuf *
carrick_icon_factory_get_pixbuf_for_state (CarrickIconFactory *factory,
                                           CarrickIconState    state)
{
  CarrickIconFactoryPrivate *priv = ICON_FACTORY_PRIVATE (factory);
  GdkPixbuf *icon = NULL;

  switch (state)
  {
    case ICON_ACTIVE:
      if (!priv->active_img)
      {
        priv->active_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->active_img;
      break;
    case ICON_ACTIVE_HOVER:
      if (!priv->active_hov_img)
      {
        priv->active_hov_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->active_hov_img;
      break;
    case ICON_CONNECTING:
      if (!priv->connecting_img)
      {
        priv->connecting_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->connecting_img;
      break;
    case ICON_CONNECTING_HOVER:
      if (!priv->connecting_hov_img)
      {
        priv->connecting_hov_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->connecting_hov_img;
      break;
    case ICON_ERROR:
      if (!priv->error_img)
      {
        priv->error_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->error_img;
      break;
    case ICON_ERROR_HOVER:
      if (!priv->error_hov_img)
      {
        priv->error_hov_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->error_hov_img;
      break;
    case ICON_OFFLINE:
      if (!priv->offline_img)
      {
        priv->offline_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->offline_img;
      break;
    case ICON_OFFLINE_HOVER:
      if (!priv->offline_hov_img)
      {
        priv->offline_hov_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->offline_hov_img;
      break;
    case ICON_WIRELESS_NETWORK_WEAK:
      if (!priv->wireless_weak_img)
      {
        priv->wireless_weak_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->wireless_weak_img;
      break;
    case ICON_WIRELESS_NETWORK_WEAK_HOVER:
      if (!priv->wireless_weak_hov_img)
      {
        priv->wireless_weak_hov_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->wireless_weak_hov_img;
      break;
    case ICON_WIRELESS_NETWORK_GOOD:
      if (!priv->wireless_good_img)
      {
        priv->wireless_good_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->wireless_good_img;
      break;
    case ICON_WIRELESS_NETWORK_GOOD_HOVER:
      if (!priv->wireless_good_hov_img)
      {
        priv->wireless_good_hov_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->wireless_good_hov_img;
      break;
    case ICON_WIRELESS_NETWORK_STRONG:
      if (!priv->wireless_strong_img)
      {
        priv->wireless_strong_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->wireless_strong_img;
      break;
    case ICON_WIRELESS_NETWORK_STRONG_HOVER:
      if (!priv->wireless_strong_hov_img)
      {
        priv->wireless_strong_hov_img =
          gdk_pixbuf_new_from_file (icon_names[state],
                                    NULL);
      }
      icon = priv->wireless_strong_hov_img;
      break;
    default:
      icon = NULL;
      break;
  }

  return icon;
}

GdkPixbuf *
carrick_icon_factory_get_pixbuf_for_service (CarrickIconFactory *factory,
                                             CmService          *service)
{
  CarrickIconState icon_state;

  icon_state = carrick_icon_factory_get_state_for_service (service) + 1;

  return carrick_icon_factory_get_pixbuf_for_state (factory,
                                                    icon_state);
}
