
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

#include <glib/gi18n.h>

#include "mpd-devices-pane.h"
#include "mpd-devices-tile.h"
#include "mpd-shell-defines.h"

G_DEFINE_TYPE (MpdDevicesPane, mpd_devices_pane, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_DEVICES_PANE, MpdDevicesPanePrivate))

enum
{
  REQUEST_HIDE,
  REQUEST_SHOW,

  LAST_SIGNAL
};

typedef struct
{
    int dummy;
    MplPanelClient *panel_client;
    MpdDevicesTile *devices_tile;
} MpdDevicesPanePrivate;

static unsigned int _signals[LAST_SIGNAL] = { 0, };

static void
_tile_request_hide_cb (ClutterActor   *tile,
                       MpdDevicesPane *self)
{
  g_signal_emit_by_name (self, "request-hide");
}

static void
_tile_request_show_cb (ClutterActor   *tile,
                       MpdDevicesPane *self)
{
  g_signal_emit_by_name (self, "request-show");
}

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_devices_pane_parent_class)->dispose (object);
}

static void
mpd_devices_pane_class_init (MpdDevicesPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdDevicesPanePrivate));

  object_class->dispose = _dispose;

  /* Signals */

  _signals[REQUEST_HIDE] = g_signal_new ("request-hide",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);

  _signals[REQUEST_SHOW] = g_signal_new ("request-show",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);
}

static void
mpd_devices_pane_init (MpdDevicesPane *self)
{
  MpdDevicesPanePrivate *priv = GET_PRIVATE (self);
  ClutterActor  *label;
  ClutterActor  *tile;

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self), MX_ORIENTATION_VERTICAL);
  clutter_actor_set_width (CLUTTER_ACTOR (self), MPD_DEVICES_PANE_WIDTH);

  label = mx_label_new_with_text (_("Other devices"));
  clutter_actor_set_name (label, "pane-label");
  clutter_container_add_actor (CLUTTER_CONTAINER (self), label);

  tile = mpd_devices_tile_new ();
  priv->devices_tile = tile;
  g_signal_connect (tile, "request-hide",
                    G_CALLBACK (_tile_request_hide_cb), self);
  g_signal_connect (tile, "request-show",
                    G_CALLBACK (_tile_request_show_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), tile);
  clutter_container_child_set (CLUTTER_CONTAINER (self), tile,
                               "expand", true,
                               NULL);
}

ClutterActor *
mpd_devices_pane_new (void)
{
  return g_object_new (MPD_TYPE_DEVICES_PANE, NULL);
}

void
mpd_devices_pane_set_client (MpdDevicesPane *self, MplPanelClient *client)
{
  MpdDevicesPanePrivate  *priv = GET_PRIVATE (self);

  priv->panel_client = client;

  mpd_devices_tile_set_client (priv->devices_tile, client);
}
