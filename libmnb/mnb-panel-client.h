/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-panel-client.h */
/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _MNB_PANEL_CLIENT
#define _MNB_PANEL_CLIENT

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MNB_TYPE_PANEL_CLIENT mnb_panel_client_get_type()

#define MNB_PANEL_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_PANEL_CLIENT, MnbPanelClient))

#define MNB_PANEL_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_PANEL_CLIENT, MnbPanelClientClass))

#define MNB_IS_PANEL_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_PANEL_CLIENT))

#define MNB_IS_PANEL_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_PANEL_CLIENT))

#define MNB_PANEL_CLIENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_PANEL_CLIENT, MnbPanelClientClass))

typedef struct _MnbPanelClientPrivate MnbPanelClientPrivate;

typedef struct {
  ClutterActor parent;

  MnbPanelClientPrivate *priv;
} MnbPanelClient;

typedef struct {
  ClutterActorClass parent_class;

} MnbPanelClientClass;

GType mnb_panel_client_get_type (void);

MnbPanelClient *mnb_panel_client_new (const gchar  *dbus_path);

G_END_DECLS

#endif /* _MNB_PANEL_CLIENT */

