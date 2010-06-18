/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Meego Netbook
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

#ifndef _MNB_ALTTAB_OVERLAY_APP
#define _MNB_ALTTAB_OVERLAY_APP

#include <mx/mx.h>

G_BEGIN_DECLS

/*
 * MnbAlttabOverlayApp
 *
 */
#define MNB_TYPE_ALTTAB_OVERLAY_APP                 (mnb_alttab_overlay_app_get_type ())
#define MNB_ALTTAB_OVERLAY_APP(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_ALTTAB_OVERLAY_APP, MnbAlttabOverlayApp))
#define MNB_IS_ALTTAB_OVERLAY_APP(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_ALTTAB_OVERLAY_APP))
#define MNB_ALTTAB_OVERLAY_APP_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_ALTTAB_OVERLAY_APP, MnbAlttabOverlayAppClass))
#define MNB_IS_ALTTAB_OVERLAY_APP_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_ALTTAB_OVERLAY_APP))
#define MNB_ALTTAB_OVERLAY_APP_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_ALTTAB_OVERLAY_APP, MnbAlttabOverlayAppClass))

typedef struct _MnbAlttabOverlayApp        MnbAlttabOverlayApp;
typedef struct _MnbAlttabOverlayAppPrivate MnbAlttabOverlayAppPrivate;
typedef struct _MnbAlttabOverlayAppClass   MnbAlttabOverlayAppClass;

struct _MnbAlttabOverlayApp
{
  /*< private >*/
  MxWidget parent;

  MnbAlttabOverlayAppPrivate *priv;
};

struct _MnbAlttabOverlayAppClass
{
  /*< private >*/
  MxWidgetClass parent_class;
};

GType mnb_alttab_overlay_app_get_type (void);

MnbAlttabOverlayApp *mnb_alttab_overlay_app_new (MutterWindow   *mw,
                                                 ClutterActor   *background);

void          mnb_alttab_overlay_app_set_active (MnbAlttabOverlayApp *app,
                                                 gboolean             active);
gboolean      mnb_alttab_overlay_app_get_active (MnbAlttabOverlayApp *app);
MutterWindow *mnb_alttab_overlay_app_get_mcw    (MnbAlttabOverlayApp *app);

G_END_DECLS

#endif /* _MNB_ALTTAB_OVERLAY_APP */
