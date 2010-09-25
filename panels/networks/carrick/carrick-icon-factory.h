/*
 * Carrick - a connection panel for the MeeGo Netbook
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

#ifndef _CARRICK_ICON_FACTORY_H
#define _CARRICK_ICON_FACTORY_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CARRICK_TYPE_ICON_FACTORY carrick_icon_factory_get_type ()

#define CARRICK_ICON_FACTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                               CARRICK_TYPE_ICON_FACTORY, CarrickIconFactory))

#define CARRICK_ICON_FACTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
                            CARRICK_TYPE_ICON_FACTORY, CarrickIconFactoryClass))

#define CARRICK_IS_ICON_FACTORY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                               CARRICK_TYPE_ICON_FACTORY))

#define CARRICK_IS_ICON_FACTORY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                            CARRICK_TYPE_ICON_FACTORY))

#define CARRICK_ICON_FACTORY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                              CARRICK_TYPE_ICON_FACTORY, CarrickIconFactoryClass))

typedef enum {
  ICON_ACTIVE,
  ICON_ACTIVE_HOVER,
  ICON_CONNECTING,
  ICON_CONNECTING_HOVER,
  ICON_ERROR,
  ICON_ERROR_HOVER,
  ICON_OFFLINE,
  ICON_OFFLINE_HOVER,
  ICON_WIRELESS_WEAK,
  ICON_WIRELESS_WEAK_HOVER,
  ICON_WIRELESS_GOOD,
  ICON_WIRELESS_GOOD_HOVER,
  ICON_WIRELESS_STRONG,
  ICON_WIRELESS_STRONG_HOVER,
  ICON_WIMAX_STRONG,
  ICON_WIMAX_STRONG_HOVER,
  ICON_WIMAX_WEAK,
  ICON_WIMAX_WEAK_HOVER,
  ICON_3G_STRONG,
  ICON_3G_STRONG_HOVER,
  ICON_3G_WEAK,
  ICON_3G_WEAK_HOVER,
  ICON_BLUETOOTH_STRONG,
  ICON_BLUETOOTH_STRONG_HOVER,
  ICON_BLUETOOTH_WEAK,
  ICON_BLUETOOTH_WEAK_HOVER,
  ICON_VPN,
  ICON_VPN_HOVER
} CarrickIconState;

typedef struct _CarrickIconFactory CarrickIconFactory;
typedef struct _CarrickIconFactoryClass CarrickIconFactoryClass;
typedef struct _CarrickIconFactoryPrivate CarrickIconFactoryPrivate;

struct _CarrickIconFactory
{
  GObject parent;
  CarrickIconFactoryPrivate *priv;
};

struct _CarrickIconFactoryClass
{
  GObjectClass parent_class;
};

GType carrick_icon_factory_get_type (void);

CarrickIconFactory* carrick_icon_factory_new (void);

GdkPixbuf *carrick_icon_factory_get_pixbuf_for_state (CarrickIconFactory *factory,
                                                      CarrickIconState    state);

const gchar *carrick_icon_factory_get_path_for_state (CarrickIconState state);

const gchar *carrick_icon_factory_get_name_for_state (CarrickIconState state);

CarrickIconState carrick_icon_factory_get_state      (const gchar *connection_type,
                                                      guint        strength);

G_END_DECLS

#endif /* _CARRICK_ICON_FACTORY_H */
