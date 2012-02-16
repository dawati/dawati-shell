/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

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

#ifndef _MNB_PANEL_FRAME
#define _MNB_PANEL_FRAME

#include <glib-object.h>
#include <clutter/clutter.h>
#include <mx/mx.h>

/*
 * These must match the assets !
 */
#define MNB_PANEL_FRAME_TOP_PADDING   4
#define MNB_PANEL_FRAME_SHADOW_HEIGHT 8

G_BEGIN_DECLS

#define MNB_TYPE_PANEL_FRAME mnb_panel_frame_get_type()

#define MNB_PANEL_FRAME(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_PANEL_FRAME, MnbPanelFrame))

#define MNB_PANEL_FRAME_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_PANEL_FRAME, MnbPanelFrameClass))

#define MPL_IS_FRAME_CLUTTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_PANEL_FRAME))

#define MPL_IS_FRAME_CLUTTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_PANEL_FRAME))

#define MNB_PANEL_FRAME_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_PANEL_FRAME, MnbPanelFrameClass))

typedef struct _MnbPanelFramePrivate MnbPanelFramePrivate;

typedef struct {
  MxWidget parent;

  /*< private >*/
  MnbPanelFramePrivate *priv;
} MnbPanelFrame;

typedef struct {
  MxWidgetClass parent_class;
} MnbPanelFrameClass;

GType             mnb_panel_frame_get_type  (void);

MnbPanelFrame * mnb_panel_frame_new       (void);
void            mnb_panel_frame_set_child (MnbPanelFrame *frame, ClutterActor *child);

G_END_DECLS

#endif /* _MNB_PANEL_FRAME */


