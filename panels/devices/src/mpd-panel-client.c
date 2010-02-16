
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
#include "mpd-panel-client.h"

G_DEFINE_TYPE (MpdPanelClient, mpd_panel_client, MPL_TYPE_PANEL_CLUTTER)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_PANEL_CLIENT, MpdPanelClientPrivate))

typedef struct
{
  int dummy;
} MpdPanelClientPrivate;

static void
mpd_panel_client_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mpd_panel_client_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mpd_panel_client_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_panel_client_parent_class)->dispose (object);
}

static void
mpd_panel_client_class_init (MpdPanelClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdPanelClientPrivate));

  object_class->get_property = mpd_panel_client_get_property;
  object_class->set_property = mpd_panel_client_set_property;
  object_class->dispose = mpd_panel_client_dispose;
}

static void
mpd_panel_client_init (MpdPanelClient *self)
{
}

MplPanelClient *
mpd_panel_client_new (const gchar *name,
                      const gchar *tooltip,
                      const gchar *button_style)
{
  return g_object_new (MPD_TYPE_PANEL_CLIENT, NULL);

  return g_object_new (MPD_TYPE_PANEL_CLIENT,
                       "name",            name,
                       "tooltip",         tooltip,
                       "button-style",    button_style,
                       "toolbar-service", true,
                       NULL);
}

