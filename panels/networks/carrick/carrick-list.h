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

#ifndef _CARRICK_LIST_H
#define _CARRICK_LIST_H

#include <gtk/gtk.h>

#include "carrick-icon-factory.h"
#include "carrick-notification-manager.h"
#include "carrick-network-model.h"

G_BEGIN_DECLS

#define CARRICK_TYPE_LIST carrick_list_get_type ()

#define CARRICK_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                               CARRICK_TYPE_LIST, CarrickList))

#define CARRICK_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
                            CARRICK_TYPE_LIST, CarrickListClass))

#define CARRICK_IS_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                               CARRICK_TYPE_LIST))

#define CARRICK_IS_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                            CARRICK_TYPE_LIST))

#define CARRICK_LIST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                              CARRICK_TYPE_LIST, CarrickListClass))

typedef struct _CarrickList CarrickList;
typedef struct _CarrickListClass CarrickListClass;
typedef struct _CarrickListPrivate CarrickListPrivate;

struct _CarrickList
{
  GtkScrolledWindow   parent;
  CarrickListPrivate *priv;
};

struct _CarrickListClass
{
  GtkScrolledWindowClass parent_class;
};

GType carrick_list_get_type (void);

GtkWidget* carrick_list_new (CarrickIconFactory         *icon_factory,
                             CarrickNotificationManager *notifications,
                             CarrickNetworkModel        *model);

void carrick_list_clear_state (CarrickList *list);

void carrick_list_set_icon_factory (CarrickList        *list,
                                    CarrickIconFactory *icon_factory);
CarrickIconFactory *carrick_list_get_icon_factory (CarrickList *list);

void carrick_list_set_notification_manager (CarrickList                *list,
                                            CarrickNotificationManager *notification_manager);
CarrickNotificationManager *carrick_list_get_notification_manager (CarrickList *list);

G_END_DECLS

#endif /* _CARRICK_LIST_H */
