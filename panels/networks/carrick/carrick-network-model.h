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

#ifndef _CARRICK_NETWORK_MODEL_H
#define _CARRICK_NETWORK_MODEL_H

#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>

#define CONNMAN_SERVICE           "net.connman"
#define CONNMAN_MANAGER_PATH      "/"
#define CONNMAN_MANAGER_INTERFACE   CONNMAN_SERVICE ".Manager"
#define CONNMAN_SERVICE_INTERFACE CONNMAN_SERVICE ".Service"

G_BEGIN_DECLS

#define CARRICK_TYPE_NETWORK_MODEL carrick_network_model_get_type ()

#define CARRICK_NETWORK_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                               CARRICK_TYPE_NETWORK_MODEL, CarrickNetworkModel))

#define CARRICK_NETWORK_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
                            CARRICK_TYPE_NETWORK_MODEL, CarrickNetworkModelClass))

#define CARRICK_IS_NETWORK_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                               CARRICK_TYPE_NETWORK_MODEL))

#define CARRICK_IS_NETWORK_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                            CARRICK_TYPE_NETWORK_MODEL))

#define CARRICK_NETWORK_MODEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                              CARRICK_TYPE_NETWORK_MODEL, CarrickNetworkModelClass))

typedef struct _CarrickNetworkModel CarrickNetworkModel;
typedef struct _CarrickNetworkModelClass CarrickNetworkModelClass;
typedef struct _CarrickNetworkModelPrivate CarrickNetworkModelPrivate;

enum {
  CARRICK_COLUMN_PROXY,
  CARRICK_COLUMN_INDEX,
  CARRICK_COLUMN_NAME,
  CARRICK_COLUMN_TYPE,
  CARRICK_COLUMN_STATE,
  CARRICK_COLUMN_FAVORITE,
  CARRICK_COLUMN_STRENGTH,
  CARRICK_COLUMN_SECURITY,
  CARRICK_COLUMN_PASSPHRASE_REQUIRED,
  CARRICK_COLUMN_PASSPHRASE,
  CARRICK_COLUMN_SETUP_REQUIRED,
  CARRICK_COLUMN_METHOD,
  CARRICK_COLUMN_ADDRESS,
  CARRICK_COLUMN_NETMASK,
  CARRICK_COLUMN_GATEWAY,
  CARRICK_COLUMN_CONFIGURED_METHOD,
  CARRICK_COLUMN_CONFIGURED_ADDRESS,
  CARRICK_COLUMN_CONFIGURED_NETMASK,
  CARRICK_COLUMN_CONFIGURED_GATEWAY,
  CARRICK_COLUMN_NAMESERVERS,
  CARRICK_COLUMN_CONFIGURED_NAMESERVERS,
  CARRICK_COLUMN_IMMUTABLE,
  CARRICK_COLUMN_LOGIN_REQUIRED,
};

struct _CarrickNetworkModel
{
  GtkListStore parent;

  CarrickNetworkModelPrivate *priv;
};

struct _CarrickNetworkModelClass
{
  GtkListStoreClass parent_class;
};

GType carrick_network_model_get_type (void);

GtkTreeModel *carrick_network_model_new (void);
DBusGProxy *carrick_network_model_get_proxy (CarrickNetworkModel *model);

G_END_DECLS

#endif /* _CARRICK_NETWORK_MODEL_H */
