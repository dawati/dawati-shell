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

#ifndef _CARRICK_SERVICE_ITEM_H
#define _CARRICK_SERVICE_ITEM_H

#include <gtk/gtk.h>
#include "carrick-icon-factory.h"
#include "carrick-notification-manager.h"
#include "carrick-network-model.h"

G_BEGIN_DECLS

#define CARRICK_TYPE_SERVICE_ITEM carrick_service_item_get_type ()

#define CARRICK_SERVICE_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                               CARRICK_TYPE_SERVICE_ITEM, CarrickServiceItem))

#define CARRICK_SERVICE_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
                            CARRICK_TYPE_SERVICE_ITEM, CarrickServiceItemClass))

#define CARRICK_IS_SERVICE_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                               CARRICK_TYPE_SERVICE_ITEM))

#define CARRICK_IS_SERVICE_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                            CARRICK_TYPE_SERVICE_ITEM))

#define CARRICK_SERVICE_ITEM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                              CARRICK_TYPE_SERVICE_ITEM, CarrickServiceItemClass))

typedef struct _CarrickServiceItem CarrickServiceItem;
typedef struct _CarrickServiceItemClass CarrickServiceItemClass;
typedef struct _CarrickServiceItemPrivate CarrickServiceItemPrivate;

struct _CarrickServiceItem
{
  GtkEventBox parent;

  CarrickServiceItemPrivate *priv;
};

struct _CarrickServiceItemClass
{
  GtkEventBoxClass parent_class;
};

GType carrick_service_item_get_type (void);

gint carrick_service_item_get_order (CarrickServiceItem *item);
gboolean carrick_service_item_get_active (CarrickServiceItem *item);
void carrick_service_item_set_active (CarrickServiceItem *item,
                                      gboolean            active);

gboolean carrick_service_item_get_favorite (CarrickServiceItem *item);
DBusGProxy* carrick_service_item_get_proxy (CarrickServiceItem *item);
GtkTreeRowReference* carrick_service_item_get_row_reference (CarrickServiceItem *item);

void carrick_service_item_update (CarrickServiceItem *self);

GtkWidget* carrick_service_item_new (CarrickIconFactory         *icon_factory,
                                     CarrickNotificationManager *notifications,
                                     CarrickNetworkModel        *model,
                                     GtkTreePath                *path);

G_END_DECLS

#endif /* _CARRICK_SERVICE_ITEM_H */
