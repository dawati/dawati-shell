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

#include "carrick-applet.h"

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "carrick-pane.h"
#include "carrick-list.h"
#include "carrick-icon-factory.h"
#include "carrick-notification-manager.h"

G_DEFINE_TYPE (CarrickApplet, carrick_applet, G_TYPE_OBJECT)

#define APPLET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_APPLET, CarrickAppletPrivate))

struct _CarrickAppletPrivate
{
  GtkWidget *pane;
  CarrickIconFactory *icon_factory;
  CarrickNotificationManager *notifications;
};

GtkWidget*
carrick_applet_get_pane (CarrickApplet *applet)
{
  CarrickAppletPrivate *priv = applet->priv;

  return priv->pane;
}

static void
carrick_applet_class_init (CarrickAppletClass *klass)
{
  g_type_class_add_private (klass, sizeof (CarrickAppletPrivate));
}

static void
carrick_applet_init (CarrickApplet *applet)
{
  CarrickAppletPrivate *priv;

  priv = applet->priv = APPLET_PRIVATE (applet);

  priv->icon_factory = carrick_icon_factory_new ();
  priv->notifications = carrick_notification_manager_new ();
  priv->pane = carrick_pane_new (priv->icon_factory,
                                 priv->notifications);
  gtk_widget_show (priv->pane);
}

CarrickApplet*
carrick_applet_new (void)
{
  return g_object_new (CARRICK_TYPE_APPLET, NULL);
}
