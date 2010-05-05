
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Bradford <rob@linux.intel.com> (dalston-button-monitor.c)
 *          Rob Staudinger <robert.staudinger@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdbool.h>

#include <glib/gi18n.h>

#include "mpd-shutdown-notification.h"
#include "config.h"

G_DEFINE_TYPE (MpdShutdownNotification, mpd_shutdown_notification, NOTIFY_TYPE_NOTIFICATION)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_SHUTDOWN_NOTIFICATION, MpdShutdownNotificationPrivate))

enum
{
  SHUTDOWN,

  LAST_SIGNAL
};

typedef struct
{
  unsigned int timeout_id;
  unsigned int countdown;
} MpdShutdownNotificationPrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static void
_notification_shutdown_cb (MpdShutdownNotification  *self,
                           char                     *action,
                           void                     *data)
{
  g_signal_emit_by_name (self, "shutdown");
}

static void
_notification_closed_cb (MpdShutdownNotification  *self,
                         void                     *data)
{
  MpdShutdownNotificationPrivate *priv = GET_PRIVATE (self);

  g_source_remove (priv->timeout_id);
  priv->timeout_id = 0;
}

static bool
_timeout_cb (MpdShutdownNotification *self)
{
  MpdShutdownNotificationPrivate *priv = GET_PRIVATE (self);
  char const  *template = _("If you don't decide I'll turn off in %d seconds");
  char        *text = NULL;
  bool         proceed;

  if (priv->countdown > 5)
  {
    /* Count down in steps of five. */
    priv->countdown -= 5;
    text = g_strdup_printf (template, priv->countdown);
    proceed = true;

    if (priv->countdown == 5) {
      /* Switch to per-second steps. */
      priv->timeout_id = g_timeout_add_seconds (1,
                                                (GSourceFunc) _timeout_cb,
                                                self);
      proceed = false;
    }

  } else if (priv->countdown > 0) {
    /* Count down in per-second steps. */
    priv->countdown--;
    text = g_strdup_printf (template, priv->countdown);
    proceed = true;
  } else {
    g_signal_emit_by_name (self, "shutdown");
    proceed = false;
  }

  if (text)
  {
    g_object_set (self,
                  "body", text,
                  NULL);
    g_free (text);
    notify_notification_show (NOTIFY_NOTIFICATION (self), NULL);
  }

  return proceed;
}

static GObject *
_constructor (GType                  type,
              unsigned int           n_properties,
              GObjectConstructParam *properties)
{
  MpdShutdownNotification *self = (MpdShutdownNotification *)
                        G_OBJECT_CLASS (mpd_shutdown_notification_parent_class)
                          ->constructor (type, n_properties, properties);

  notify_notification_set_urgency (NOTIFY_NOTIFICATION (self),
                                   NOTIFY_URGENCY_CRITICAL);
  notify_notification_set_timeout (NOTIFY_NOTIFICATION (self),
                                   NOTIFY_EXPIRES_NEVER);
  notify_notification_add_action (NOTIFY_NOTIFICATION (self),
                                  "meego:XF86XK_PowerOff",
                                  _("Turn off"),
                                  NOTIFY_ACTION_CALLBACK (_notification_shutdown_cb),
                                  NULL, NULL);

  g_signal_connect (self, "closed",
                    G_CALLBACK (_notification_closed_cb), NULL);

  return (GObject *) self;
}

static void
_dispose (GObject *object)
{
  MpdShutdownNotificationPrivate *priv = GET_PRIVATE (object);

  if (priv->timeout_id)
  {
    g_source_remove (priv->timeout_id);
    priv->timeout_id = 0;
  }

  G_OBJECT_CLASS (mpd_shutdown_notification_parent_class)->dispose (object);
}

static void
mpd_shutdown_notification_class_init (MpdShutdownNotificationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdShutdownNotificationPrivate));

  object_class->constructor = _constructor;
  object_class->dispose = _dispose;

  /* Signals */

  _signals[SHUTDOWN] = g_signal_new ("shutdown",
                                     G_TYPE_FROM_CLASS (klass),
                                     G_SIGNAL_RUN_LAST,
                                     0, NULL, NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE, 0);
}

static void
mpd_shutdown_notification_init (MpdShutdownNotification *self)
{
}

NotifyNotification *
mpd_shutdown_notification_new (char const *summary,
                               char const *body)
{
  return g_object_new (MPD_TYPE_SHUTDOWN_NOTIFICATION,
                       "summary", summary,
                       "body", body,
                       "icon-name", "system-shutdown",
                       NULL);
}

void
mpd_shutdown_notification_run (MpdShutdownNotification *self)
{
  MpdShutdownNotificationPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (priv->timeout_id == 0);

  priv->countdown = 30;
  priv->timeout_id = g_timeout_add_seconds (5,
                                            (GSourceFunc) _timeout_cb,
                                            self);

  notify_notification_show (NOTIFY_NOTIFICATION (self), NULL);
}

