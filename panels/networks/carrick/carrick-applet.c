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

#include "carrick-applet.h"

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gconnman/gconnman.h>
#include "carrick-pane.h"
#include "carrick-status-icon.h"
#include "carrick-list.h"
#include "carrick-icon-factory.h"
#include "carrick-notification-manager.h"

G_DEFINE_TYPE (CarrickApplet, carrick_applet, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_APPLET, CarrickAppletPrivate))

typedef struct _CarrickAppletPrivate CarrickAppletPrivate;

struct _CarrickAppletPrivate {
  CmManager          *manager;
  GtkWidget          *icon;
  GtkWidget          *pane;
  gchar              *state;
  CarrickIconFactory *icon_factory;
  CarrickNotificationManager *notifications;
};

static void
manager_services_changed_cb (CmManager *manager,
			     gpointer   user_data)
{
  CarrickApplet *applet = CARRICK_APPLET (user_data);
  CarrickAppletPrivate *priv = GET_PRIVATE (applet);

  if (priv->icon)
    carrick_status_icon_update (CARRICK_STATUS_ICON (priv->icon));
}

static void
manager_state_changed_cb (CmManager *manager,
                          gpointer   user_data)
{
  CarrickApplet *applet = CARRICK_APPLET (user_data);
  CarrickAppletPrivate *priv = GET_PRIVATE (applet);
  gchar *new_state = g_strdup (cm_manager_get_state (manager));

  if (g_strcmp0 (priv->state, new_state) != 0)
  {
    g_free (priv->state);
    priv->state = new_state;
  }

  if (priv->icon)
    carrick_status_icon_update (CARRICK_STATUS_ICON (priv->icon));
}

GtkWidget*
carrick_applet_get_pane (CarrickApplet *applet)
{
  CarrickAppletPrivate *priv = GET_PRIVATE (applet);

  return priv->pane;
}

GtkWidget*
carrick_applet_get_icon (CarrickApplet *applet)
{
  CarrickAppletPrivate *priv = GET_PRIVATE (applet);

  return priv->icon;
}

static void
carrick_applet_dispose (GObject *object)
{
  G_OBJECT_CLASS (carrick_applet_parent_class)->dispose (object);
}

static void
carrick_applet_finalize (GObject *object)
{
  G_OBJECT_CLASS (carrick_applet_parent_class)->finalize (object);
}

static void
carrick_applet_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
carrick_applet_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
carrick_applet_class_init (CarrickAppletClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CarrickAppletPrivate));

  object_class->dispose = carrick_applet_dispose;
  object_class->finalize = carrick_applet_finalize;
  object_class->get_property = carrick_applet_get_property;
  object_class->set_property = carrick_applet_set_property;
}

static void
carrick_applet_init (CarrickApplet *self)
{
  CarrickAppletPrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;

  priv->manager = cm_manager_new (&error);
  if (error || !priv->manager) {
    g_debug ("Error initializing connman manager: %s\n",
             error->message);
    /* FIXME: must do better here */
    return;
  }
  /* FIXME: handle return value here */
  cm_manager_refresh (priv->manager);
  priv->state = g_strdup (cm_manager_get_state (priv->manager));
  priv->icon_factory = carrick_icon_factory_new ();
  priv->icon = carrick_status_icon_new (priv->icon_factory,
                                        priv->manager);
  priv->notifications = carrick_notification_manager_new (priv->manager);
  priv->pane = carrick_pane_new (priv->icon_factory,
                                 priv->notifications,
                                 priv->manager);
  gtk_widget_show (priv->pane);

  g_signal_connect (priv->manager,
                    "state-changed",
                    G_CALLBACK (manager_state_changed_cb),
                    self);
  g_signal_connect (priv->manager,
                    "services-changed",
                    G_CALLBACK (manager_services_changed_cb),
                    self);
}

CarrickApplet*
carrick_applet_new (void)
{
  return g_object_new (CARRICK_TYPE_APPLET, NULL);
}
