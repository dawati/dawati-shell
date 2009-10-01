/*
 * Dalston - power and volume applets for Moblin
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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
 *
 */

#include "dalston-idle-manager.h"
#include <libegg-idletime/egg-idletime.h>
#include <gconf/gconf-client.h>

G_DEFINE_TYPE (DalstonIdleManager, dalston_idle_manager, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_IDLE_MANAGER, DalstonIdleManagerPrivate))

typedef struct _DalstonIdleManagerPrivate DalstonIdleManagerPrivate;

struct _DalstonIdleManagerPrivate {
  EggIdletime *idletime;
  GConfClient *client;
  guint suspend_idle_time_notify_id;
};

#define MOBLIN_GCONF_DIR "/desktop/moblin"
#define SUSPEND_IDLE_TIME_KEY "suspend_idle_time"

#define SUSPEND_ALARM_ID 1

static void
dalston_idle_manager_dispose (GObject *object)
{
  DalstonIdleManagerPrivate *priv = GET_PRIVATE (object);

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

  G_OBJECT_CLASS (dalston_idle_manager_parent_class)->dispose (object);
}

static void
dalston_idle_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (dalston_idle_manager_parent_class)->finalize (object);
}

static void
dalston_idle_manager_class_init (DalstonIdleManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DalstonIdleManagerPrivate));

  object_class->dispose = dalston_idle_manager_dispose;
  object_class->finalize = dalston_idle_manager_finalize;
}

static void
_suspend_idle_time_key_changed_cb (GConfClient *client,
                                   guint        cnxn_id,
                                   GConfEntry  *entry,
                                   gpointer     userdata)
{
  DalstonIdleManager *manager = DALSTON_IDLE_MANAGER (userdata);
  DalstonIdleManagerPrivate *priv = GET_PRIVATE (manager);
  gint suspend_idle_time_minutes = -1;
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

  egg_idletime_alarm_remove (priv->idletime, SUSPEND_ALARM_ID);

  if (suspend_idle_time_minutes > 0)
  {
    egg_idletime_alarm_set (priv->idletime,
                            SUSPEND_ALARM_ID,
                            suspend_idle_time_minutes * 60 * 1000);
  }
}

static void
_idletime_alarm_expired_cb (EggIdletime *idletime,
                            guint        alarm_id,
                            gpointer     userdata)
{
  DalstonIdleManager *manager = DALSTON_IDLE_MANAGER (userdata);
  DalstonIdleManagerPrivate *priv = GET_PRIVATE (manager);

  if (alarm_id == SUSPEND_ALARM_ID)
  {
    g_debug (G_STRLOC ": Got supend on idle alarm event");
  }
}

static void
dalston_idle_manager_init (DalstonIdleManager *self)
{
  DalstonIdleManagerPrivate *priv = GET_PRIVATE (self);
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
}

DalstonIdleManager *
dalston_idle_manager_new (void)
{
  return g_object_new (DALSTON_TYPE_IDLE_MANAGER, NULL);
}


