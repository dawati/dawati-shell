
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
#include <gconf/gconf-client.h>

#include "mpd-gobject.h"
#include "mpd-idle-manager.h"
#include "config.h"

G_DEFINE_TYPE (MpdIdleManager, mpd_idle_manager, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_IDLE_MANAGER, MpdIdleManagerPrivate))

typedef struct
{
  GConfClient *client;
  unsigned int suspend_idle_time_notify_id;

  int suspend_idle_time;

  DBusGProxy *presence;
  DBusGProxy *screensaver;

  DkpClient *power_client;

  guint suspend_source_id;
} MpdIdleManagerPrivate;

#define MOBLIN_GCONF_DIR "/desktop/moblin"
#define SUSPEND_IDLE_TIME_KEY MOBLIN_GCONF_DIR"/suspend_idle_time"

static void
_dispose (GObject *object)
{
  MpdIdleManagerPrivate *priv = GET_PRIVATE (object);

  if (priv->suspend_idle_time_notify_id)
  {
    gconf_client_notify_remove (priv->client,
                                priv->suspend_idle_time_notify_id);
    priv->suspend_idle_time_notify_id = 0;
  }

  mpd_gobject_detach (object, (GObject **) &priv->client);
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

static gboolean
_suspend_timer_elapsed (gpointer data)
{
  MpdIdleManager *self;
  MpdIdleManagerPrivate *priv;
  GError *error = NULL;

  g_return_val_if_fail (MPD_IS_IDLE_MANAGER (data), FALSE);

  self = MPD_IDLE_MANAGER (data);
  priv = GET_PRIVATE (self);

  priv->suspend_source_id = 0;

  g_debug ("Suspending after %d seconds of idle",
           priv->suspend_idle_time);
  if (!mpd_idle_manager_suspend (self, &error))
  {
    g_warning (G_STRLOC ": Unable to suspend: %s\n", error->message);
    g_clear_error (&error);
  }

  return FALSE;
}


#define MIN_SUSPEND_DELAY 15
static void
_presence_status_changed (DBusGProxy *presence,
                          guint       status,
                          gpointer    data)
{
  MpdIdleManager *self;
  MpdIdleManagerPrivate *priv;

  g_return_if_fail (MPD_IS_IDLE_MANAGER (data));

  self = MPD_IDLE_MANAGER (data);
  priv = GET_PRIVATE (self);

  if (status == 3)
  {
    if (priv->suspend_source_id == 0)
    {
      /* session just became idle */
      if (priv->suspend_idle_time >= 0 &&
          priv->suspend_idle_time < MIN_SUSPEND_DELAY)
      {
        /* suspend now but leave a few seconds for gnome-screensaver:
         * otherwise may get a screensave _after_ resuming. This
         * also lets screensaver finish any fading it wants to do,
         * not to mention giving the user a few seconds to break idle */
        priv->suspend_source_id =
          g_timeout_add_seconds (MIN_SUSPEND_DELAY,
                                 _suspend_timer_elapsed,
                                 data);
      }
      else if (priv->suspend_idle_time > 0)
      {
        priv->suspend_source_id =
          g_timeout_add_seconds (priv->suspend_idle_time,
                                 _suspend_timer_elapsed,
                                 data);
      }
    }
  }
  else if (priv->suspend_source_id > 0)
  {
    /* session just became non-idle and we have a timer */
    g_source_remove (priv->suspend_source_id);
    priv->suspend_source_id = 0;
  }
}

static void
_suspend_idle_time_key_changed_cb (GConfClient  *client,
                                   unsigned int  cnxn_id,
                                   GConfEntry   *entry,
                                   void         *data)
{
  MpdIdleManagerPrivate *priv;

  g_return_if_fail (MPD_IS_IDLE_MANAGER (data));
  priv = GET_PRIVATE (data);

  priv->suspend_idle_time = gconf_client_get_int (priv->client,
                                                  SUSPEND_IDLE_TIME_KEY,
                                                  NULL);
}

static void
mpd_idle_manager_init (MpdIdleManager *self)
{
  MpdIdleManagerPrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;
  DBusGConnection *conn;

  priv->suspend_idle_time = -1;
  priv->power_client = dkp_client_new ();
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
                             SUSPEND_IDLE_TIME_KEY,
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
                                 G_CALLBACK (_presence_status_changed),
                                 self, NULL);

    priv->screensaver =
      dbus_g_proxy_new_for_name (conn,
                                 "org.gnome.ScreenSaver",
                                 "/",
                                 "org.gnome.ScreenSaver");
  }

  gconf_client_notify (priv->client, SUSPEND_IDLE_TIME_KEY);
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
  MpdIdleManagerPrivate *priv;

  g_return_val_if_fail (MPD_IS_IDLE_MANAGER (self), FALSE);
  priv = GET_PRIVATE (self);

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
  if (!ret || (error && *error))
  {
    return false;
  }

  ret = dkp_client_suspend (priv->power_client, error);
  dbus_g_proxy_call_no_reply (priv->screensaver, "SimulateUserActivity",
                              G_TYPE_INVALID);
  if (!ret || (error && *error))
  {
    return false;
  }

  return true;
}

