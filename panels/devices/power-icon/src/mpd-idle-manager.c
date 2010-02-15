
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
#include <egg-idletime/egg-idletime.h>
#include <gconf/gconf-client.h>
#include "mpd-idle-manager.h"

G_DEFINE_TYPE (MpdIdleManager, mpd_idle_manager, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_IDLE_MANAGER, MpdIdleManagerPrivate))

typedef struct
{
  EggIdletime *idletime;
  GConfClient *client;
  guint suspend_idle_time_notify_id;
  DkpClient *power_client;
} MpdIdleManagerPrivate;

#define MOBLIN_GCONF_DIR "/desktop/moblin"
#define SUSPEND_IDLE_TIME_KEY "suspend_idle_time"

#define SUSPEND_ALARM_ID 1

static void
mpd_idle_manager_dispose (GObject *object)
{
  MpdIdleManagerPrivate *priv = GET_PRIVATE (object);

  if (priv->idletime)
  {
    g_object_unref (priv->idletime);
    priv->idletime = NULL;
  }

  if (priv->suspend_idle_time_notify_id)
  {
    gconf_client_notify_remove (priv->client,
                                priv->suspend_idle_time_notify_id);
    priv->suspend_idle_time_notify_id = 0;
  }

  if (priv->client)
  {
    g_object_unref (priv->client);
    priv->client = NULL;
  }

  if (priv->power_client)
  {
    g_object_unref (priv->power_client);
    priv->power_client = NULL;
  }

  G_OBJECT_CLASS (mpd_idle_manager_parent_class)->dispose (object);
}

static void
mpd_idle_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (mpd_idle_manager_parent_class)->finalize (object);
}

static void
mpd_idle_manager_class_init (MpdIdleManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdIdleManagerPrivate));

  object_class->dispose = mpd_idle_manager_dispose;
  object_class->finalize = mpd_idle_manager_finalize;
}

static void
_set_suspend_idle_alarm (MpdIdleManager *manager)
{
  MpdIdleManagerPrivate *priv = GET_PRIVATE (manager);
  int suspend_idle_time_minutes = -1;
  GError *error = NULL;

  suspend_idle_time_minutes =
    gconf_client_get_int (priv->client,
                          MOBLIN_GCONF_DIR"/"SUSPEND_IDLE_TIME_KEY,
                          &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error getting suspend idle time gconf key: %s",
               error->message);
    g_clear_error (&error);
    suspend_idle_time_minutes = -1;
  }


  if (suspend_idle_time_minutes > 0)
  {
    egg_idletime_alarm_set (priv->idletime,
                            SUSPEND_ALARM_ID,
                            suspend_idle_time_minutes * 60 * 1000);
  }
}

static void
_suspend_idle_time_key_changed_cb (GConfClient *client,
                                   guint        cnxn_id,
                                   GConfEntry  *entry,
                                   gpointer     userdata)
{
  MpdIdleManager *manager = MPD_IDLE_MANAGER (userdata);
  MpdIdleManagerPrivate *priv = GET_PRIVATE (manager);

  egg_idletime_alarm_remove (priv->idletime, SUSPEND_ALARM_ID);

  _set_suspend_idle_alarm (manager);
}

static void
_idletime_alarm_expired_cb (EggIdletime     *idletime,
                            guint            alarm_id,
                            MpdIdleManager  *self)
{
  GError *error = NULL;

  if (alarm_id == SUSPEND_ALARM_ID)
  {
    g_debug (G_STRLOC ": Got suspend on idle alarm event");

    mpd_idle_manager_suspend (self, &error);
    if (error)
    {
      g_warning ("%s : %s", G_STRLOC, error->message);
      g_clear_error (&error);
    }

    _set_suspend_idle_alarm (self);
  }
}

static void
mpd_idle_manager_init (MpdIdleManager *self)
{
  MpdIdleManagerPrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;

  priv->idletime = egg_idletime_new ();
  priv->client = gconf_client_get_default ();

  gconf_client_add_dir (priv->client,
                        MOBLIN_GCONF_DIR,
                        GCONF_CLIENT_PRELOAD_NONE,
                        &error);

  if (error)
  {
    g_warning (G_STRLOC ": Unable to add gconf client directory: %s",
               error->message);
    g_clear_error (&error);
  }

  priv->suspend_idle_time_notify_id =
    gconf_client_notify_add (priv->client,
                             MOBLIN_GCONF_DIR"/"SUSPEND_IDLE_TIME_KEY,
                             _suspend_idle_time_key_changed_cb,
                             g_object_ref (self),
                             g_object_unref,
                             &error);

  if (error)
  {
    g_warning (G_STRLOC ": Unable to add gconf client key notify: %s",
               error->message);
    g_clear_error (&error);
  }

  gconf_client_notify (priv->client,
                       MOBLIN_GCONF_DIR"/"SUSPEND_IDLE_TIME_KEY);

  g_signal_connect (priv->idletime,
                    "alarm-expired",
                    (GCallback)_idletime_alarm_expired_cb,
                    self);

  priv->power_client = dkp_client_new ();
}

MpdIdleManager *
mpd_idle_manager_new (void)
{
  return g_object_new (MPD_TYPE_IDLE_MANAGER, NULL);
}

bool
mpd_idle_manager_lock_screen (MpdIdleManager   *self,
                              GError          **error)
{
  DBusGConnection *conn;
  DBusGProxy *proxy;

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, error);
  if (error && *error)
  {
    return false;
  }

  proxy = dbus_g_proxy_new_for_name (conn,
                                     "org.gnome.ScreenSaver",
                                     "/",
                                     "org.gnome.ScreenSaver");

  dbus_g_proxy_call_no_reply (proxy, "Lock", G_TYPE_INVALID);

  g_object_unref (proxy);
  return true;
}

bool
mpd_idle_manager_suspend (MpdIdleManager   *self,
                          GError          **error)
{
  MpdIdleManagerPrivate *priv = GET_PRIVATE (self);
  bool ret;

  ret = mpd_idle_manager_lock_screen (self, error);
  if (!ret || (error && *error))
  {
    return false;
  }

  ret = dkp_client_suspend (priv->power_client, error);
  if (!ret || (error && *error))
  {
    return false;
  }

  return true;
}

