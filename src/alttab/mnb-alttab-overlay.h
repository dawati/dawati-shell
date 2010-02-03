/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Moblin Netbook
 * Copyright © 2009, 2010, Intel Corporation.
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

#ifndef _MNB_ALTTAB_OVERLAY
#define _MNB_ALTTAB_OVERLAY

#include <mx/mx.h>
#include "mnb-alttab.h"

#include  <mutter-window.h>

#define MNB_ALTTAB_OVERLAY_TILE_WIDTH   200
#define MNB_ALTTAB_OVERLAY_TILE_HEIGHT  130
#define MNB_ALTTAB_OVERLAY_TILE_SPACING  10
#define MNB_ALLTAB_OVERLAY_COLUMNS        4
#define MNB_ALLTAB_OVERLAY_ROWS           4

G_BEGIN_DECLS

/*
 * MnbAlttabOverlay
 *
 * A MnbAlttabItem subclass represening a single application thumb in the alttab.
 *
 */
#define MNB_TYPE_ALTTAB_OVERLAY                 (mnb_alttab_overlay_get_type ())
#define MNB_ALTTAB_OVERLAY(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_ALTTAB_OVERLAY, MnbAlttabOverlay))
#define MNB_IS_ALTTAB_OVERLAY(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_ALTTAB_OVERLAY))
#define MNB_ALTTAB_OVERLAY_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_ALTTAB_OVERLAY, MnbAlttabOverlayClass))
#define MNB_IS_ALTTAB_OVERLAY_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_ALTTAB_OVERLAY))
#define MNB_ALTTAB_OVERLAY_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_ALTTAB_OVERLAY, MnbAlttabOverlayClass))

typedef struct _MnbAlttabOverlay               MnbAlttabOverlay;
typedef struct _MnbAlttabOverlayPrivate        MnbAlttabOverlayPrivate;
typedef struct _MnbAlttabOverlayClass          MnbAlttabOverlayClass;

struct _MnbAlttabOverlay
{
  /*< private >*/
  MxWidget parent_instance;

  MnbAlttabOverlayPrivate *priv;
};

struct _MnbAlttabOverlayClass
{
  /*< private >*/
  MxWidgetClass parent_class;
};

GType mnb_alttab_overlay_get_type (void);

MnbAlttabOverlay *mnb_alttab_overlay_new (void);

/*
 * Xevent hook for the alt+tab code
 */
gboolean    mnb_alttab_overlay_handle_xevent (MnbAlttabOverlay *overlay,
                                              XEvent             *xev);

gboolean mnb_alttab_overlay_show (MnbAlttabOverlay *overlay, gboolean backward);
void     mnb_alttab_overlay_hide (MnbAlttabOverlay *overlay);
void     mnb_alttab_overlay_activate_window (MnbAlttabOverlay *overlay,
                                             MutterWindow       *activate,
                                             guint               timestamp);
void     mnb_alttab_reset_autoscroll (MnbAlttabOverlay *overlay,
                                      gboolean          backward);

G_END_DECLS

#endif /* _MNB_ALTTAB_OVERLAY */
