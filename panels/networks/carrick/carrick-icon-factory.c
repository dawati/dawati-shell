/*
 * Carrick - a connection panel for the Moblin Netbook
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Written by - Joshua Lock <josh@linux.intel.com>
 *
 */

#include "carrick-icon-factory.h"
#include <config.h>

G_DEFINE_TYPE (CarrickIconFactory, carrick_icon_factory, G_TYPE_OBJECT)

#define ICON_FACTORY_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_ICON_FACTORY, CarrickIconFactoryPrivate))

#define PKG_ICON_DIR PKG_DATA_DIR "/" "icons"

static const gchar * icon_names[] = {
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
  PKG_ICON_DIR "/" "network-wireless-signal-strong-hover.png",
  PKG_ICON_DIR "/" "network-wimax-strong.png",
  PKG_ICON_DIR "/" "network-wimax-strong-hover.png",
  PKG_ICON_DIR "/" "network-wimax-weak.png",
  PKG_ICON_DIR "/" "network-wimax-weak-hover.png",
  PKG_ICON_DIR "/" "network-3g-strong.png",
  PKG_ICON_DIR "/" "network-3g-strong-hover.png",
  PKG_ICON_DIR "/" "network-3g-weak.png",
  PKG_ICON_DIR "/" "network-3g-weak-hover.png",
  PKG_ICON_DIR "/" "network-bluetooth-strong.png",
  PKG_ICON_DIR "/" "network-bluetooth-strong-hover.png",
  PKG_ICON_DIR "/" "network-bluetooth-weak.png",
  PKG_ICON_DIR "/" "network-bluetooth-weak-hover.png"
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
  GdkPixbuf *wimax_strong_img;
  GdkPixbuf *wimax_strong_hov_img;
  GdkPixbuf *wimax_weak_img;
  GdkPixbuf *wimax_weak_hov_img;
  GdkPixbuf *threeg_strong_img;
  GdkPixbuf *threeg_strong_hov_img;
  GdkPixbuf *threeg_weak_img;
  GdkPixbuf *threeg_weak_hov_img;
  GdkPixbuf *bluetooth_strong_img;
  GdkPixbuf *bluetooth_strong_hov_img;
  GdkPixbuf *bluetooth_weak_img;
  GdkPixbuf *bluetooth_weak_hov_img;
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
  CarrickIconFactoryPrivate *priv;

  priv = self->priv = ICON_FACTORY_PRIVATE (self);

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
  priv->wimax_strong_img = NULL;
  priv->wimax_strong_hov_img = NULL;
  priv->wimax_weak_img = NULL;
  priv->wimax_weak_hov_img = NULL;
  priv->threeg_strong_img = NULL;
  priv->threeg_strong_hov_img = NULL;
  priv->threeg_weak_img = NULL;
  priv->threeg_weak_hov_img = NULL;
  priv->bluetooth_strong_img = NULL;
  priv->bluetooth_strong_hov_img = NULL;
  priv->bluetooth_weak_img = NULL;
  priv->bluetooth_weak_hov_img = NULL;
}

CarrickIconFactory*
carrick_icon_factory_new (void)
{
  return g_object_new (CARRICK_TYPE_ICON_FACTORY, NULL);
}
const gchar *
carrick_icon_factory_get_path_for_state (CarrickIconState state)
{
  return icon_names[state];
}

GdkPixbuf *
carrick_icon_factory_get_pixbuf_for_state (CarrickIconFactory *factory,
                                           CarrickIconState    state)
{
  CarrickIconFactoryPrivate *priv = factory->priv;
  GdkPixbuf                 *icon = NULL;
  GError                    *error = NULL;

  switch (state)
    {
    case ICON_ACTIVE:
      if (!priv->active_img)
        {
          priv->active_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->active_img;
      break;

    case ICON_ACTIVE_HOVER:
      if (!priv->active_hov_img)
        {
          priv->active_hov_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->active_hov_img;
      break;

    case ICON_CONNECTING:
      if (!priv->connecting_img)
        {
          priv->connecting_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->connecting_img;
      break;

    case ICON_CONNECTING_HOVER:
      if (!priv->connecting_hov_img)
        {
          priv->connecting_hov_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->connecting_hov_img;
      break;

    case ICON_ERROR:
      if (!priv->error_img)
        {
          priv->error_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->error_img;
      break;

    case ICON_ERROR_HOVER:
      if (!priv->error_hov_img)
        {
          priv->error_hov_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->error_hov_img;
      break;

    case ICON_OFFLINE:
      if (!priv->offline_img)
        {
          priv->offline_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->offline_img;
      break;

    case ICON_OFFLINE_HOVER:
      if (!priv->offline_hov_img)
        {
          priv->offline_hov_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->offline_hov_img;
      break;

    case ICON_WIRELESS_WEAK:
      if (!priv->wireless_weak_img)
        {
          priv->wireless_weak_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->wireless_weak_img;
      break;

    case ICON_WIRELESS_WEAK_HOVER:
      if (!priv->wireless_weak_hov_img)
        {
          priv->wireless_weak_hov_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->wireless_weak_hov_img;
      break;

    case ICON_WIRELESS_GOOD:
      if (!priv->wireless_good_img)
        {
          priv->wireless_good_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->wireless_good_img;
      break;

    case ICON_WIRELESS_GOOD_HOVER:
      if (!priv->wireless_good_hov_img)
        {
          priv->wireless_good_hov_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->wireless_good_hov_img;
      break;

    case ICON_WIRELESS_STRONG:
      if (!priv->wireless_strong_img)
        {
          priv->wireless_strong_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->wireless_strong_img;
      break;

    case ICON_WIRELESS_STRONG_HOVER:
      if (!priv->wireless_strong_hov_img)
        {
          priv->wireless_strong_hov_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->wireless_strong_hov_img;
      break;

    case ICON_WIMAX_STRONG:
      if (!priv->wimax_strong_img)
        {
          priv->wimax_strong_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->wimax_strong_img;
      break;

    case ICON_WIMAX_STRONG_HOVER:
      if (!priv->wimax_strong_hov_img)
        {
          priv->wimax_strong_hov_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->wimax_strong_hov_img;
      break;

    case ICON_WIMAX_WEAK:
      if (!priv->wimax_weak_img)
        {
          priv->wimax_weak_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->wimax_weak_img;
      break;

    case ICON_WIMAX_WEAK_HOVER:
      if (!priv->wimax_weak_hov_img)
        {
          priv->wimax_weak_hov_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->wimax_weak_hov_img;
      break;

    case ICON_3G_STRONG:
      if (!priv->threeg_strong_img)
        {
          priv->threeg_strong_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->threeg_strong_img;
      break;

    case ICON_3G_STRONG_HOVER:
      if (!priv->threeg_strong_hov_img)
        {
          priv->threeg_strong_hov_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->threeg_strong_hov_img;
      break;

    case ICON_3G_WEAK:
      if (!priv->threeg_weak_img)
        {
          priv->threeg_weak_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->threeg_weak_img;
      break;

    case ICON_3G_WEAK_HOVER:
      if (!priv->threeg_weak_hov_img)
        {
          priv->threeg_weak_hov_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->threeg_weak_hov_img;
      break;

    case ICON_BLUETOOTH_STRONG:
      if (!priv->bluetooth_strong_img)
        {
          priv->bluetooth_strong_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->bluetooth_strong_img;
      break;

    case ICON_BLUETOOTH_STRONG_HOVER:
      if (!priv->bluetooth_strong_hov_img)
        {
          priv->bluetooth_strong_hov_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->bluetooth_strong_hov_img;
      break;

    case ICON_BLUETOOTH_WEAK:
      if (!priv->bluetooth_weak_img)
        {
          priv->bluetooth_weak_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->bluetooth_weak_img;
      break;

    case ICON_BLUETOOTH_WEAK_HOVER:
      if (!priv->bluetooth_weak_hov_img)
        {
          priv->bluetooth_weak_hov_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->bluetooth_weak_hov_img;
      break;

    default:
      if (!priv->error_img)
        {
          priv->error_img =
            gdk_pixbuf_new_from_file (icon_names[state],
                                      &error);
        }
      icon = priv->error_img;
      break;
    }

  if (icon == NULL || error)
    {
      g_warning (G_STRLOC ":error opening pixbuf: %s",
                 error->message);
      g_clear_error (&error);
    }

  return icon;
}
