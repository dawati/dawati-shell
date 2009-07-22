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

#include "carrick-notification-manager.h"

#include <config.h>
#include <libnotify/notify.h>
#include <glib/gi18n.h>

#include "carrick-icon-factory.h"

G_DEFINE_TYPE (CarrickNotificationManager, carrick_notification_manager, G_TYPE_OBJECT)

#define NOTIFICATION_MANAGER_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_NOTIFICATION_MANAGER, CarrickNotificationManagerPrivate))

struct _CarrickNotificationManagerPrivate
{
  CmManager *manager;
  gchar *last_type;
  gchar *last_name;
  gchar *last_state;

  gchar *queued_type;
  gchar *queued_name;
  gchar *queued_state;
};

enum
{
  PROP_0,
  PROP_MANAGER
};

void
carrick_notification_manager_queue_service (CarrickNotificationManager *self,
                                            CmService *service,
                                            gboolean enabling)
{
  CarrickNotificationManagerPrivate *priv = self->priv;
  const gchar *name = cm_service_get_name (service);
  const gchar *type = cm_service_get_type (service);

  g_free (priv->queued_state);
  if (enabling)
    priv->queued_state = g_strdup ("ready");
  else
    priv->queued_state = g_strdup ("idle");

  g_free (priv->queued_type);
  priv->queued_type = g_strdup (type);

  if (name)
  {
    g_free (priv->queued_name);
    priv->queued_name = g_strdup (name);
  }
}

void
carrick_notification_manager_queue_event (CarrickNotificationManager *self,
                                          const gchar *type,
                                          const gchar *state,
                                          const gchar *name)
{
  CarrickNotificationManagerPrivate *priv = self->priv;
  g_free (priv->queued_type);
  g_free (priv->queued_state);
  g_free (priv->queued_name);

  if (type)
    priv->queued_type = g_strdup (type);

  if (state)
    priv->queued_state = g_strdup (state);

  if (name)
    priv->queued_name = g_strdup (name);
}

static void
_send_note (gchar *title,
            gchar *message,
            const gchar *icon)
{
  NotifyNotification *note;

  note = notify_notification_new (title,
                                  message,
                                  icon,
                                  NULL);

  notify_notification_set_timeout (note,
                                   10000);
  notify_notification_show (note,
                            NULL);

  g_object_unref (note);
}

static void
_tell_online (const gchar *name,
              const gchar *type,
              guint str)
{
  gchar *title = g_strdup (_("Network connected"));
  gchar *message = NULL;
  const gchar *icon = NULL;

  if (g_strcmp0 (type, "ethernet") == 0)
  {
    icon = carrick_icon_factory_get_path_for_state (ICON_ACTIVE);

    message = g_strdup_printf (_("You're now connected to a wired network"));
  }
  else
  {
    if (g_strcmp0 (type, "wifi") == 0)
    {
      if (str > 70)
        icon = carrick_icon_factory_get_path_for_state (ICON_WIRELESS_STRONG);
      else if (str > 35)
        icon = carrick_icon_factory_get_path_for_state (ICON_WIRELESS_GOOD);
      else
        icon = carrick_icon_factory_get_path_for_state (ICON_WIRELESS_WEAK);
    }
    else if (g_strcmp0 (type, "wimax") == 0)
    {
      if (str > 50)
        icon = carrick_icon_factory_get_path_for_state (ICON_WIMAX_STRONG);
      else
        icon = carrick_icon_factory_get_path_for_state (ICON_WIMAX_WEAK);
    }
    else if (g_strcmp0 (type, "cellular") == 0)
    {
      if (str > 50)
        icon = carrick_icon_factory_get_path_for_state (ICON_3G_STRONG);
      else
        icon = carrick_icon_factory_get_path_for_state (ICON_3G_WEAK);
    }

    if (name && name[0] != '\0')
    {
      message = g_strdup_printf (_("You're now connected to %s, a %s network"),
                                 name,
                                 type);
    }
    else
    {
      message = g_strdup_printf (_("You're now connected to a %s network"),
                                 type);
    }
  }

  _send_note (title, message, icon);

  g_free (title);
  g_free (message);
}

static void
_tell_offline (CarrickNotificationManager *self,
               const gchar *name,
               const gchar *type)
{
  CarrickNotificationManagerPrivate *priv = self->priv;
  gchar *title = g_strdup (_("Network lost"));
  gchar *message = NULL;
  const gchar *icon;

  icon = carrick_icon_factory_get_path_for_state (ICON_OFFLINE);

  if (g_strcmp0 (priv->last_type, "ethernet") == 0)
  {
    message = g_strdup_printf (_("Sorry, we've lost your wired connection"));
  }
  else if (priv->last_name)
  {
    message = g_strdup_printf (_("Sorry we've lost your %s connection to %s"),
                               priv->last_type,
                               priv->last_name);
  }
  else if (priv->last_type)
  {
    message = g_strdup_printf (_("Sorry, we've lost your %s connection"),
                               priv->last_type);
  }

  _send_note (title, message, icon);

  g_free (title);
  g_free (message);
}

static void
_tell_changed (CarrickNotificationManager *self,
               const gchar *name,
               const gchar *type,
               guint str)
{
  CarrickNotificationManagerPrivate *priv = self->priv;
  gchar *title = g_strdup (_("Network changed"));
  gchar *old = NULL;
  gchar *new = NULL;
  gchar *message = NULL;
  const gchar *icon;

  if (g_strcmp0 (priv->last_type, "ethernet") == 0)
  {
    old = g_strdup (_("Sorry, your wired connection was lost. So we've "));
  }
  else if (priv->last_name)
  {
    old = g_strdup_printf (_("Sorry, your connection to %s was lost. So we've "),
                           name);
  }
  else
  {
    old = g_strdup_printf (_("Sorry, your %s connection was lost. So we've "),
                           type);
  }

  if (g_strcmp0 (type, "ethernet") == 0)
  {
    new = g_strdup (_("connected you to a wired network"));
    icon = carrick_icon_factory_get_path_for_state (ICON_ACTIVE);
  }
  else if (name)
  {
    new = g_strdup_printf (_("connected you to %s, a %s network"),
                           name,
                           type);
  }
  else
  {
    new = g_strdup_printf (_("connected you to a %s network"),
                           type);
  }

  if (g_strcmp0 (type, "wifi") == 0)
  {
    if (str > 70)
      icon = carrick_icon_factory_get_path_for_state (ICON_WIRELESS_STRONG);
    else if (str > 35)
      icon = carrick_icon_factory_get_path_for_state (ICON_WIRELESS_GOOD);
    else
      icon = carrick_icon_factory_get_path_for_state (ICON_WIRELESS_WEAK);
  }
  else if (g_strcmp0 (type, "wimax") == 0)
  {
    if (str > 50)
      icon = carrick_icon_factory_get_path_for_state (ICON_WIMAX_STRONG);
    else
      icon = carrick_icon_factory_get_path_for_state (ICON_WIMAX_WEAK);
  }
  else if (g_strcmp0 (type, "cellular") == 0)
  {
    if (str > 50)
      icon = carrick_icon_factory_get_path_for_state (ICON_3G_STRONG);
    else
      icon = carrick_icon_factory_get_path_for_state (ICON_3G_WEAK);
  }

  message = g_strdup_printf ("%s %s", old, new);

  _send_note (title, message, icon);

  g_free (old);
  g_free (new);
  g_free (title);
  g_free (message);
}

static void
_service_state_stash_cb (CmService *service,
                         CarrickNotificationManager *self)
{
  CarrickNotificationManagerPrivate *priv = self->priv;

  g_signal_handlers_disconnect_by_func (service,
                                        _service_state_stash_cb,
                                        self);

  /* If the fist service is connected and no state saved then save it */
  if (cm_service_get_connected (service) && !priv->last_state)
  {
    priv->last_state = g_strdup (cm_service_get_state (service));
    priv->last_name = g_strdup (cm_service_get_name (service));
    priv->last_type = g_strdup (cm_service_get_type (service));
  }
}

static void
_services_changed_cb (CmManager *manager,
                      CarrickNotificationManager *self)
{
  CarrickNotificationManagerPrivate *priv = self->priv;
  const GList *new_services = cm_manager_get_services (manager);
  CmService *new_top;
  const gchar *type = NULL;
  const gchar *name = NULL;
  const gchar *state = NULL;
  guint str = 0;
  gboolean queue_handled = FALSE;

  if (!new_services)
    return;

  new_top = new_services->data;

  if (!new_top)
    return;

  type = cm_service_get_type (new_top);
  name = cm_service_get_name (new_top);
  state = cm_service_get_state (new_top);
  str = cm_service_get_strength (new_top);

  /* FIXME: handle offline mode */

  /*
   * Determine what note to send, we can:
   * _tell_changed (self, name, type, str)
   * _tell_offline (self, name, type)
   * _tell_online (name, type, str)
   */

  /* Need to handle last events and queued events separately to better maintain
   * the systems state */
  if (priv->queued_state)
  {
    /* We have a queued event, test to see if that's what happened */
    if (g_strcmp0 (priv->queued_type, type) == 0 &&
        g_strcmp0 (priv->queued_state, state) == 0)
    {
      g_free (priv->queued_state);
      priv->queued_state = NULL;
      g_free (priv->queued_type);
      priv->queued_type = NULL;
      g_free (priv->queued_name);
      priv->queued_name = NULL;
      queue_handled = TRUE;
    }
  }

  if (!queue_handled &&
      (g_strcmp0 (priv->last_type, type) != 0 ||
       (priv->last_name != NULL && g_strcmp0 (priv->last_name, name) != 0)))
  {
    /* top service has changed */
    if (g_strcmp0 (state, "ready") == 0 &&
        g_strcmp0 (priv->last_state, "idle") == 0)
    {
      _tell_online (name, type, str);
    }
    else if (g_strcmp0 (state, "ready") == 0 &&
             g_strcmp0 (priv->last_state, "ready") == 0
             && g_strcmp0 (name, priv->last_name) != 0)
    {
      _tell_changed (self, name, type, str);
    }
    else if (g_strcmp0 (state, "idle") == 0
             && g_strcmp0 (priv->last_state, "ready") == 0)
    {
      _tell_offline (self, name, type);
    }
  }
  else if (!queue_handled &&
           g_strcmp0 (priv->last_name, name) == 0 &&
           g_strcmp0 (priv->last_state, state) != 0)
  {
    /* service same but state changed */
    if (g_strcmp0 (state, "ready") == 0)
      _tell_online (name, type, str);
    else if (g_strcmp0 (state, "idle") == 0)
      _tell_offline (self, name, type);
  }

  /*
   * Stash state in last_*
   */
  g_free (priv->last_state);
  priv->last_state = g_strdup (state);

  if (g_strcmp0 (state, "ready") == 0)
  {
    g_free (priv->last_type);
    priv->last_type = g_strdup (type);
    g_free (priv->last_name);
    priv->last_name = g_strdup (name);
  }
}

static void
_set_manager (CarrickNotificationManager *self,
              CmManager *manager)
{
  CarrickNotificationManagerPrivate *priv = self->priv;
  const GList *services;

  if (priv->manager)
  {
    g_signal_handlers_disconnect_by_func (priv->manager,
                                          _services_changed_cb,
                                          self);
    g_object_unref (priv->manager);
    priv->manager = NULL;
  }

  if (manager)
  {
    priv->manager = g_object_ref (manager);
    g_signal_connect (priv->manager,
                      "services-changed",
                      G_CALLBACK (_services_changed_cb),
                      self);

    services = cm_manager_get_services (priv->manager);
    if (services)
    {
      g_signal_connect (CM_SERVICE (services->data),
                        "service-updated",
                        G_CALLBACK (_service_state_stash_cb),
                        self);
    }
  }
}

static void
carrick_notification_manager_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  CarrickNotificationManager *notif = CARRICK_NOTIFICATION_MANAGER (object);
  CarrickNotificationManagerPrivate *priv = notif->priv;

  switch (property_id)
  {
  case PROP_MANAGER:
    g_value_set_object (value, priv->manager);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
carrick_notification_manager_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  CarrickNotificationManager *notif = CARRICK_NOTIFICATION_MANAGER (object);

  switch (property_id)
  {
  case PROP_MANAGER:
    _set_manager (notif,
                  CM_MANAGER (g_value_get_object (value)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
carrick_notification_manager_dispose (GObject *object)
{

  CarrickNotificationManager *self = CARRICK_NOTIFICATION_MANAGER (object);
  CarrickNotificationManagerPrivate *priv = self->priv;

  notify_uninit ();

  g_free (priv->last_type);
  g_free (priv->last_name);
  g_free (priv->last_state);
  g_free (priv->queued_type);
  g_free (priv->queued_name);
  g_free (priv->queued_state);

  if (priv->manager)
  {
    _set_manager (self,
                  NULL);
  }

  G_OBJECT_CLASS (carrick_notification_manager_parent_class)->dispose (object);
}

static void
carrick_notification_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (carrick_notification_manager_parent_class)->finalize (object);
}

static void
carrick_notification_manager_class_init (CarrickNotificationManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (CarrickNotificationManagerPrivate));

  object_class->get_property = carrick_notification_manager_get_property;
  object_class->set_property = carrick_notification_manager_set_property;
  object_class->dispose = carrick_notification_manager_dispose;
  object_class->finalize = carrick_notification_manager_finalize;

  pspec = g_param_spec_object ("manager",
                               "CmManager",
                               "CmManager object to watch for changed services",
                               CM_TYPE_MANAGER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_MANAGER,
                                   pspec);
}

static void
carrick_notification_manager_init (CarrickNotificationManager *self)
{
  self->priv = NOTIFICATION_MANAGER_PRIVATE (self);

  self->priv->manager = NULL;
  self->priv->last_type = NULL;
  self->priv->last_name = NULL;
  self->priv->last_state = NULL;
  self->priv->queued_type = NULL;
  self->priv->queued_name = NULL;
  self->priv->queued_state = NULL;

  notify_init ("Carrick");
}

CarrickNotificationManager *
carrick_notification_manager_new (CmManager *manager)
{
  return g_object_new (CARRICK_TYPE_NOTIFICATION_MANAGER,
                       "manager",
                       manager,
                       NULL);
}
