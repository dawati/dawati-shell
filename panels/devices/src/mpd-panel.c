
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
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

#include "mpd-panel.h"
#include "mpd-shell-defines.h"
#include "config.h"

static void _container_iface_init (ClutterContainerIface *iface);

G_DEFINE_TYPE_WITH_CODE (MpdPanel, mpd_panel, MPL_TYPE_PANEL_CLUTTER,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                _container_iface_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_PANEL, MpdPanelPrivate))

typedef struct
{
  int dummy;
} MpdPanelPrivate;

/*
 * ClutterContainerIface
 * FIXME: Implement the whole interface by forwarding to the stage.
 */

static void
_container_iface_add (ClutterContainer *container,
                      ClutterActor     *actor)
{
  ClutterActor *stage;

  stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (container));
  g_return_if_fail (stage);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), actor);
}

static void
_container_iface_remove (ClutterContainer *container,
                         ClutterActor     *actor)
{
  ClutterActor *stage;

  stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (container));
  g_return_if_fail (stage);

  clutter_container_remove_actor (CLUTTER_CONTAINER (stage), actor);
}

static void
_container_iface_foreach (ClutterContainer  *container,
                          ClutterCallback    callback,
                          void              *data)
{
  ClutterActor *stage;

  stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (container));
  g_return_if_fail (stage);

  clutter_container_foreach (CLUTTER_CONTAINER (stage), callback, data);
}

static void
_container_iface_init (ClutterContainerIface *iface)
{
  iface->add = _container_iface_add;
  iface->remove = _container_iface_remove;
  iface->foreach = _container_iface_foreach;
}

/*
 * MpdPanel
 */

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_panel_parent_class)->dispose (object);
}

static void
mpd_panel_class_init (MpdPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdPanelPrivate));

  object_class->dispose = _dispose;
}

static void
mpd_panel_init (MpdPanel *self)
{
  mpl_panel_client_set_height_request (MPL_PANEL_CLIENT (self),
                                       MPD_SHELL_HEIGHT);
}

MplPanelClient *
mpd_panel_new (const gchar *name,
               const gchar *tooltip,
               const gchar *button_style)
{
  return g_object_new (MPD_TYPE_PANEL,
                       "name",            name,
                       "tooltip",         tooltip,
                       "button-style",    button_style,
                       "toolbar-service", true,
                       NULL);
}

