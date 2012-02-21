/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * MnbLauncherRunning - keep track of running applications to match against
 * applications in the dawati application panel
 *
 * Copyright © 2012 Intel Corporation.
 *
 * Author: Michael Wood <michael.g.wood@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

/* mnb-launcher-running.c */

#include "mnb-launcher-running.h"

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

G_DEFINE_TYPE (MnbLauncherRunning, mnb_launcher_running, G_TYPE_OBJECT)

#define MNB_LAUNCHER_RUNNING_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_MNB_LAUNCHER_RUNNING, MnbLauncherRunningPrivate))

struct _MnbLauncherRunningPrivate
{
  WnckScreen *screen;
  GHashTable *current_running_table;
};


enum
{
  CHANGED,

  LAST_SIGNAL
};

static guint _signals[LAST_SIGNAL] = { 0, };

GList *
mnb_launcher_running_get_running (MnbLauncherRunning *self)
{
  MnbLauncherRunningPrivate *priv = self->priv;

  return g_hash_table_get_values (priv->current_running_table);
}

static gchar *
_window_group_name_to_desktop_file (WnckWindow *window)
{
  gchar *lowercase_name, *desktop_file;
  const gchar *group_name;

  group_name = wnck_window_get_class_group_name (window);

  if (!group_name)
    return NULL;

  lowercase_name = g_utf8_strdown (group_name, -1);

  desktop_file = g_strconcat (lowercase_name, ".desktop", NULL);

  g_free (lowercase_name);

  return desktop_file;
}

static void
_init_get_running (MnbLauncherRunning *self)
{
  MnbLauncherRunningPrivate *priv = self->priv;

  GList *windows, *iter = NULL;

  windows = wnck_screen_get_windows (priv->screen);

  for (iter = windows; iter; iter = iter->next)
    {
      WnckApplication *app;
      gchar *desktop_file;
      gint pid;

      app = wnck_window_get_application (iter->data);
      pid = wnck_application_get_pid (app);
      desktop_file = _window_group_name_to_desktop_file (iter->data);

      if (desktop_file)
        {
          g_hash_table_insert (priv->current_running_table,
                               GINT_TO_POINTER (pid),
                               desktop_file);
        }
    }
}

static void
_window_opened_cb (WnckScreen         *screen,
                   WnckWindow         *window,
                   MnbLauncherRunning *self)
{
  MnbLauncherRunningPrivate *priv = self->priv;

  gchar *desktop_file;
  gint pid;

  pid = wnck_window_get_pid (window);
  desktop_file = _window_group_name_to_desktop_file (window);

  if (desktop_file)
    {
      g_hash_table_insert (priv->current_running_table,
                           GINT_TO_POINTER (pid),
                           desktop_file);

      g_signal_emit (self, _signals[CHANGED], 0);
    }
}

static void
_window_closed_cb (WnckScreen         *screen,
                   WnckWindow         *window,
                   MnbLauncherRunning *self)
{
  MnbLauncherRunningPrivate *priv = self->priv;

  g_hash_table_remove (priv->current_running_table,
                       GINT_TO_POINTER (wnck_window_get_pid (window)));

  g_signal_emit (self, _signals[CHANGED], 0);
}

static void
mnb_launcher_running_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnb_launcher_running_parent_class)->dispose (object);
}

static void
mnb_launcher_running_finalize (GObject *object)
{
  MnbLauncherRunningPrivate *priv = MNB_MNB_LAUNCHER_RUNNING (object)->priv;

  g_hash_table_destroy (priv->current_running_table);

  G_OBJECT_CLASS (mnb_launcher_running_parent_class)->finalize (object);
}

static void
mnb_launcher_running_class_init (MnbLauncherRunningClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbLauncherRunningPrivate));

  object_class->dispose = mnb_launcher_running_dispose;
  object_class->finalize = mnb_launcher_running_finalize;

  _signals[CHANGED] = g_signal_new ("changed",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    0,
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE, 0);
}

static void
mnb_launcher_running_init (MnbLauncherRunning *self)
{
  MnbLauncherRunningPrivate *priv;

  self->priv = MNB_LAUNCHER_RUNNING_PRIVATE (self);
  priv = self->priv;

  priv->current_running_table = g_hash_table_new_full (g_direct_hash,
                                                       g_direct_equal,
                                                       NULL,
                                                       (GDestroyNotify)g_free);
  priv->screen = wnck_screen_get_default ();
  wnck_screen_force_update (priv->screen);

  _init_get_running (self);

  g_signal_connect (priv->screen, "window-opened",
                   G_CALLBACK (_window_opened_cb),
                   self);

  g_signal_connect (priv->screen, "window-closed",
                   G_CALLBACK (_window_closed_cb),
                   self);

}

MnbLauncherRunning *
mnb_launcher_running_new (void)
{
  return g_object_new (MNB_TYPE_MNB_LAUNCHER_RUNNING, NULL);
}
