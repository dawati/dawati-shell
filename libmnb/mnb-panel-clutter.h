/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-panel-clutter.h */
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

#ifndef _MNB_PANEL_CLUTTER
#define _MNB_PANEL_CLUTTER

#include <clutter/clutter.h>

#include "mnb-panel-client.h"

G_BEGIN_DECLS

#define MNB_TYPE_PANEL_CLUTTER mnb_panel_clutter_get_type()

#define MNB_PANEL_CLUTTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_PANEL_CLUTTER, MnbPanelClutter))

#define MNB_PANEL_CLUTTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_PANEL_CLUTTER, MnbPanelClutterClass))

#define MNB_IS_PANEL_CLUTTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_PANEL_CLUTTER))

#define MNB_IS_PANEL_CLUTTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_PANEL_CLUTTER))

#define MNB_PANEL_CLUTTER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_PANEL_CLUTTER, MnbPanelClutterClass))

typedef struct _MnbPanelClutterPrivate MnbPanelClutterPrivate;

typedef struct
{
  MnbPanelClient parent;

  MnbPanelClutterPrivate *priv;
} MnbPanelClutter;

typedef struct
{
  MnbPanelClientClass parent_class;

} MnbPanelClutterClass;

GType mnb_panel_clutter_get_type (void);

MnbPanelClient *mnb_panel_clutter_new   (const gchar *name,
                                         const gchar *tooltip,
                                         const gchar *stylesheet,
                                         const gchar *button_style,
                                         gboolean     with_toolbar_service);

ClutterActor *mnb_panel_clutter_get_stage (MnbPanelClutter *panel);

G_END_DECLS

#endif /* _MNB_PANEL_CLUTTER */

