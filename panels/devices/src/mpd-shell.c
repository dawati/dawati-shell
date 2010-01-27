
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Robert Staudinger <robert.staudinger@intel.com>
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

#include "mpd-computer-pane.h"
#include "mpd-folder-pane.h"
#include "mpd-shell.h"
#include "mpd-shell-defines.h"
#include "config.h"

G_DEFINE_TYPE (MpdShell, mpd_shell, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_SHELL, MpdShellPrivate))

enum
{
  REQUEST_HIDE,

  LAST_SIGNAL
};

typedef struct
{
  /* Managed by Clutter. */
  ClutterActor *folder_pane;
  ClutterActor *computer_pane;
} MpdShellPrivate;

static guint _signals[LAST_SIGNAL] = { 0, };

static void
_pane_request_hide_cb (ClutterActor *pane,
                       MpdShell     *self)
{
  g_signal_emit_by_name (self, "request-hide");
}

static void
_width_notify_cb (MpdShell    *self,
                  GParamSpec  *pspec,
                  gpointer     user_data)
{
  MpdShellPrivate *priv = GET_PRIVATE (self);
  gfloat shell_width;
  gfloat folder_pane_width;

  shell_width = clutter_actor_get_width (CLUTTER_ACTOR (self));
  folder_pane_width = shell_width - MPD_SHELL_SPACING - MPD_COMPUTER_PANE_WIDTH;
  clutter_actor_set_width (priv->folder_pane, folder_pane_width);
}

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_shell_parent_class)->dispose (object);
}

static void
mpd_shell_class_init (MpdShellClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdShellPrivate));

  object_class->dispose = _dispose;

  /* Signals */

  _signals[REQUEST_HIDE] = g_signal_new ("request-hide",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);
}

static void
mpd_shell_init (MpdShell *self)
{
  MpdShellPrivate *priv = GET_PRIVATE (self);

  mx_box_layout_set_spacing (MX_BOX_LAYOUT (self), MPD_SHELL_SPACING);
  g_signal_connect (self, "notify::width",
                    G_CALLBACK (_width_notify_cb), NULL);

  priv->folder_pane = mpd_folder_pane_new ();
  g_signal_connect (priv->folder_pane, "request-hide",
                    G_CALLBACK (_pane_request_hide_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->folder_pane);

  priv->computer_pane = mpd_computer_pane_new ();
  clutter_actor_set_width (priv->computer_pane, MPD_COMPUTER_PANE_WIDTH);
  g_signal_connect (priv->computer_pane, "request-hide",
                    G_CALLBACK (_pane_request_hide_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->computer_pane);
}

ClutterActor *
mpd_shell_new (void)
{
  return g_object_new (MPD_TYPE_SHELL, NULL);
}


