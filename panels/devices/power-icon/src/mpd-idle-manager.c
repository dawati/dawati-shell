
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Bradford <rob@linux.intel.com> (dalston-idle-manager.c)
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

#include <dbus/dbus-glib.h>
#include <devkit-power-gobject/devicekit-power.h>

#include "mpd-conf.h"
#include "mpd-gobject.h"
#include "mpd-idle-manager.h"
#include "config.h"

G_DEFINE_TYPE (MpdIdleManager, mpd_idle_manager, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_IDLE_MANAGER, MpdIdleManagerPrivate))

typedef struct
{
  MpdConf     *conf;

  DBusGProxy  *presence;
  DBusGProxy  *screensaver;

  DkpClient   *power_client;

  guint        suspend_source_id;
} MpdIdleManagerPrivate;

static void
_dispose (GObject *object)
{
  MpdIdleManagerPrivate *priv = GET_PRIVATE (object);

  mpd_gobject_detach (object, (GObject **) &priv->conf);
  mpd_gobject_detach (object, (GObject **) &priv->presence);
  mpd_gobject_detach (object, (GObject **) &priv->screensaver);
  mpd_gobject_detach (object, (GObject **) &priv->power_client);

  G_OBJECT_CLASS (mpd_idle_manager_parent_class)->dispose (object);
}

static void
mpd_idle_manager_class_init (MpdIdleManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdIdleManagerPrivate));

  object_class->dispose = _dispose;
}

static bool
_suspend_timer_elapsed_cb (MpdIdleManager *self)
{
  MpdIdleManagerPrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;

  priv->suspend_source_id = 0;

  if (!mpd_idle_manager_suspend (self, &error))
  {
    g_warning (G_STRLOC ": Unable to suspend: %s\n", error->message);
    g_clear_error (&error);
  }

  return FALSE;
}

static void
_presence_status_changed_cb (DBusGProxy     *presence,
                             unsigned int    status,
                             MpdIdleManager *self)
{
  MpdIdleManagerPrivate *priv = GET_PRIVATE (self);

  if (status == 3)
  {
    if (priv->suspend_source_id == 0)
    {
      int suspend_idle_time = mpd_conf_get_suspend_idle_time (priv->conf);

      /* session just became idle */
      if (suspend_idle_time > 0)
      {
        priv->suspend_source_id =
          g_timeout_add_seconds (suspend_idle_time,
                                 (GSourceFunc) _suspend_timer_elapsed_cb,
                                 self);
      }
    }
  } else if (priv->suspend_source_id > 0) {
    /* session just became non-idle and we have a timer */
    g_source_remove (priv->suspend_source_id);
    priv->suspend_source_id = 0;
  }
}

static void
mpd_idle_manager_init (MpdIdleManager *self)
{
  MpdIdleManagerPrivate *priv = GET_PRIVATE (self);
  DBusGConnection *conn;
  GError          *error = NULL;

  priv->conf = mpd_conf_new ();

  priv->power_client = dkp_client_new ();

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!conn)
  {
    g_warning (G_STRLOC ": Unable to connect to session bus: %s\n",
               error->message);
    g_clear_error (&error);
  }
  else
  {
    priv->presence =
      dbus_g_proxy_new_for_name (conn,
                                 "org.gnome.SessionManager",
                                 "/org/gnome/SessionManager/Presence",
                                 "org.gnome.SessionManager.Presence");

    dbus_g_proxy_add_signal (priv->presence, "StatusChanged",
                             G_TYPE_UINT, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal (priv->presence, "StatusChanged",
                                 G_CALLBACK (_presence_status_changed_cb),
                                 self, NULL);

    priv->screensaver =
      dbus_g_proxy_new_for_name (conn,
                                 "org.gnome.ScreenSaver",
                                 "/",
                                 "org.gnome.ScreenSaver");
  }
}

MpdIdleManager *
mpd_idle_manager_new (void)
{
  return g_object_new (MPD_TYPE_IDLE_MANAGER, NULL);
}

bool
mpd_idle_manager_activate_screensaver (MpdIdleManager   *self,
                                       GError          **error)
{
  MpdIdleManagerPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPD_IS_IDLE_MANAGER (self), FALSE);

  dbus_g_proxy_call_no_reply (priv->screensaver, "SetActive",
                              G_TYPE_BOOLEAN, TRUE,
                              G_TYPE_INVALID);

  return true;
}

bool
mpd_idle_manager_suspend (MpdIdleManager   *self,
                          GError          **error)
{
  MpdIdleManagerPrivate *priv = GET_PRIVATE (self);
  bool ret;

  ret = mpd_idle_manager_activate_screensaver (self, error);
  if (!ret)
    return false;

  ret = dkp_client_suspend (priv->power_client, error);

  dbus_g_proxy_call_no_reply (priv->screensaver, "SimulateUserActivity",
                              G_TYPE_INVALID);

  return ret;
}

