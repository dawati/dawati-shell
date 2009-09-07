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
  gchar *last_type;
  gchar *last_name;
  gchar *last_state;

  gchar *queued_type;
  gchar *queued_name;
  gchar *queued_state;
};

enum {
  PROP_0,
};

void
carrick_notification_manager_queue_event (CarrickNotificationManager *self,
                                          const gchar                *type,
                                          const gchar                *state,
                                          const gchar                *name)
{
  CarrickNotificationManagerPrivate *priv = self->priv;

  g_free (priv->queued_type);
  priv->queued_type = NULL;

  g_free (priv->queued_state);
  priv->queued_state = NULL;

  g_free (priv->queued_name);
  priv->queued_name = NULL;

  if (type)
    priv->queued_type = g_strdup (type);

  if (state)
    priv->queued_state = g_strdup (state);

  if (name)
    priv->queued_name = g_strdup (name);
}

static void
_send_note (gchar       *title,
            gchar       *message,
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
              guint        str)
{
  gchar       *title = g_strdup (_ ("Network connected"));
  gchar       *message = NULL;
  const gchar *icon = NULL;

  if (g_strcmp0 (type, "ethernet") == 0)
    {
      icon = carrick_icon_factory_get_path_for_state (ICON_ACTIVE);

      message = g_strdup_printf (_ ("You're now connected to a wired network"));
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
          message = g_strdup_printf (_ ("You're now connected to %s, a %s network"),
                                     name,
                                     type);
        }
      else
        {
          message = g_strdup_printf (_ ("You're now connected to a %s network"),
                                     type);
        }
    }

  _send_note (title, message, icon);

  g_free (title);
  g_free (message);
}

static void
_tell_offline (CarrickNotificationManager *self,
               const gchar                *name,
               const gchar                *type)
{
  CarrickNotificationManagerPrivate *priv = self->priv;
  gchar                             *title = g_strdup (_ ("Network lost"));
  gchar                             *message = NULL;
  const gchar                       *icon;

  icon = carrick_icon_factory_get_path_for_state (ICON_OFFLINE);

  if (g_strcmp0 (priv->last_type, "ethernet") == 0)
    {
      message = g_strdup_printf (_ ("Sorry, we've lost your wired connection"));
    }
  else if (priv->last_name)
    {
      message = g_strdup_printf (_ ("Sorry we've lost your %s connection to %s"),
                                 priv->last_type,
                                 priv->last_name);
    }
  else if (priv->last_type)
    {
      message = g_strdup_printf (_ ("Sorry, we've lost your %s connection"),
                                 priv->last_type);
    }

  _send_note (title, message, icon);

  g_free (title);
  g_free (message);
}

static void
_tell_changed (CarrickNotificationManager *self,
               const gchar                *name,
               const gchar                *type,
               guint                       str)
{
  CarrickNotificationManagerPrivate *priv = self->priv;
  gchar                             *title = g_strdup (_ ("Network changed"));
  gchar                             *message = NULL;
  const gchar                       *icon;

  if (priv->last_name == NULL && priv->last_type == NULL)
    {
      /*
       * If we have never been notified of a previous network
       * name or network type then we it would be better to just
       * not send a notification.
       */
      g_free (title);
      return;
    }

  if (priv->last_name)
    {
      if (g_strcmp0 (type, "ethernet") == 0)
        {
          message = g_strdup_printf (_ ("Sorry, your connection to %s was lost. "
                                        "So we've connected you to a wired network"),
                                     priv->last_name);
          icon = carrick_icon_factory_get_path_for_state (ICON_ACTIVE);
        }
      else if (name)
        {
          message = g_strdup_printf (_ ("Sorry, your connection to %s was lost. So "
                                        "we've connected you to %s, a %s network"),
                                     priv->last_name,
                                     name,
                                     type);
        }
      else
        {
          message = g_strdup_printf (_ ("Sorry, your connection to %s was lost. "
                                        "So we've connected you to a %s network"),
                                     priv->last_name,
                                     type);
        }
    }
  else
    {
      if (g_strcmp0 (type, "ethernet") == 0)
        {
          message = g_strdup_printf (_ ("Sorry, your %s connection was lost. "
                                        "So we've connected you to a wired network"),
                                     priv->last_type);
          icon = carrick_icon_factory_get_path_for_state (ICON_ACTIVE);
        }
      else if (name)
        {
          message = g_strdup_printf (_ ("Sorry, your %s connection was lost. "
                                        "So we've connected you to %s, a %s "
                                        "network"),
                                     priv->last_type,
                                     name,
                                     type);
        }
      else
        {
          message = g_strdup_printf (_ ("Sorry, your %s connection was lost. So "
                                        "we've connected you to a %s network"),
                                     priv->last_type,
                                     type);
        }
    }

  /* Determine icon to show in notification */
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

  _send_note (title, message, icon);

  g_free (title);
  g_free (message);
}

static void
carrick_notification_manager_dispose (GObject *object)
{
  notify_uninit ();

  G_OBJECT_CLASS (carrick_notification_manager_parent_class)->dispose (object);
}

static void
carrick_notification_manager_finalize (GObject *object)
{
  CarrickNotificationManager *self = CARRICK_NOTIFICATION_MANAGER (object);
  CarrickNotificationManagerPrivate *priv = self->priv;

  g_free (priv->last_type);
  g_free (priv->last_name);
  g_free (priv->last_state);
  g_free (priv->queued_type);
  g_free (priv->queued_name);
  g_free (priv->queued_state);

  G_OBJECT_CLASS (carrick_notification_manager_parent_class)->finalize (object);
}

static void
carrick_notification_manager_class_init (CarrickNotificationManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CarrickNotificationManagerPrivate));

  object_class->dispose = carrick_notification_manager_dispose;
  object_class->finalize = carrick_notification_manager_finalize;
}

static void
carrick_notification_manager_init (CarrickNotificationManager *self)
{
  self->priv = NOTIFICATION_MANAGER_PRIVATE (self);

  self->priv->last_type = NULL;
  self->priv->last_name = NULL;
  self->priv->last_state = NULL;
  self->priv->queued_type = NULL;
  self->priv->queued_name = NULL;
  self->priv->queued_state = NULL;

  notify_init ("Carrick");
}

CarrickNotificationManager *
carrick_notification_manager_new (void)
{
  return g_object_new (CARRICK_TYPE_NOTIFICATION_MANAGER,
                       NULL);
}
