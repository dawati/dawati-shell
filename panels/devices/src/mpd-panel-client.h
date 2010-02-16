
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

#ifndef MPD_PANEL_CLIENT_H
#define MPD_PANEL_CLIENT_H

#include <glib-object.h>
#include <moblin-panel/mpl-panel-clutter.h>

G_BEGIN_DECLS

#define MPD_TYPE_PANEL_CLIENT mpd_panel_client_get_type()

#define MPD_PANEL_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_PANEL_CLIENT, MpdPanelClient))

#define MPD_PANEL_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_PANEL_CLIENT, MpdPanelClientClass))

#define MPD_IS_PANEL_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_PANEL_CLIENT))

#define MPD_IS_PANEL_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_PANEL_CLIENT))

#define MPD_PANEL_CLIENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_PANEL_CLIENT, MpdPanelClientClass))

typedef struct
{
  MplPanelClutter parent;
} MpdPanelClient;

typedef struct
{
  MplPanelClutterClass parent;
} MpdPanelClientClass;

GType
mpd_panel_client_get_type (void);

MplPanelClient *
mpd_panel_client_new (const gchar *name,
                      const gchar *tooltip,
                      const gchar *button_style);

G_END_DECLS

#endif /* MPD_PANEL_CLIENT_H */

