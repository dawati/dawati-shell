/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009, 2010 Intel Corp.
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

#ifndef _MPL_PANEL_BACKGROUND
#define _MPL_PANEL_BACKGROUND

#include <glib-object.h>
#include <clutter/clutter.h>
#include <mx/mx.h>

/*
 * These must match the assets !
 */
#define MPL_PANEL_BACKGROUND_TOP_PADDING   4
#define MPL_PANEL_BACKGROUND_SHADOW_HEIGHT 8

G_BEGIN_DECLS

#define MPL_TYPE_PANEL_BACKGROUND mpl_panel_background_get_type()

#define MPL_PANEL_BACKGROUND(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPL_TYPE_PANEL_BACKGROUND, MplPanelBackground))

#define MPL_PANEL_BACKGROUND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPL_TYPE_PANEL_BACKGROUND, MplPanelBackgroundClass))

#define MPL_IS_BACKGROUND_CLUTTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPL_TYPE_PANEL_BACKGROUND))

#define MPL_IS_BACKGROUND_CLUTTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPL_TYPE_PANEL_BACKGROUND))

#define MPL_PANEL_BACKGROUND_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPL_TYPE_PANEL_BACKGROUND, MplPanelBackgroundClass))

typedef struct _MplPanelBackgroundPrivate MplPanelBackgroundPrivate;

typedef struct {
  MxWidget parent;

  /*< private >*/
  MplPanelBackgroundPrivate *priv;
} MplPanelBackground;

typedef struct {
  MxWidgetClass parent_class;
} MplPanelBackgroundClass;

GType             mpl_panel_background_get_type  (void);

MplPanelBackground * mpl_panel_background_new       (void);
void            mpl_panel_background_set_child (MplPanelBackground *background, ClutterActor *child);

G_END_DECLS

#endif /* _MPL_PANEL_BACKGROUND */


