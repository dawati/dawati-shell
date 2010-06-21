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
 * Written by - Ross Burton <ross@linux.intel.com>
 */

#ifndef __GGG_SERVICE_H__
#define __GGG_SERVICE_H__

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define GGG_TYPE_SERVICE (ggg_service_get_type())
#define GGG_SERVICE(obj)                                                \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                GGG_TYPE_SERVICE,                       \
                                GggService))
#define GGG_SERVICE_CLASS(klass)                                        \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             GGG_TYPE_SERVICE,                          \
                             GggServiceClass))
#define IS_GGG_SERVICE(obj)                                             \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                GGG_TYPE_SERVICE))
#define IS_GGG_SERVICE_CLASS(klass)                                     \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             GGG_TYPE_SERVICE))
#define GGG_SERVICE_GET_CLASS(obj)                                      \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               GGG_TYPE_SERVICE,                        \
                               GggServiceClass))

typedef struct _GggServicePrivate GggServicePrivate;
typedef struct _GggService      GggService;
typedef struct _GggServiceClass GggServiceClass;

struct _GggService {
  GObject parent;
  GggServicePrivate *priv;
};

struct _GggServiceClass {
  GObjectClass parent_class;
};

GType ggg_service_get_type (void) G_GNUC_CONST;

GggService * ggg_service_new (DBusGConnection *connection, const char *path);

GggService * ggg_service_new_fake (const char *name, gboolean roaming);

const char * ggg_service_get_name (GggService *service);

gboolean ggg_service_is_roaming (GggService *service);

const char * ggg_service_get_mcc (GggService *service);

const char * ggg_service_get_mnc (GggService *service);

void ggg_service_set (GggService *service,
                      const char *apn,
                      const char *username,
                      const char *password);

void ggg_service_connect (GggService *service);

G_END_DECLS

#endif /* __GGG_SERVICE_H__ */
