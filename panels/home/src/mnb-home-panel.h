/*
 * Copyright (c) 2012 Intel Corp.
 *
 * Author: Danielle Madeley <danielle.madeley@collabora.co.uk>
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

#ifndef __MNB_HOME_PANEL_H__
#define __MNB_HOME_PANEL_H__

#include <glib-object.h>
#include <mx/mx.h>
#include <dawati-panel/mpl-panel-clutter.h>
#include <dawati-panel/mpl-panel-common.h>

G_BEGIN_DECLS

#define MNB_TYPE_HOME_PANEL	(mnb_home_panel_get_type ())
#define MNB_HOME_PANEL(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_HOME_PANEL, MnbHomePanel))
#define MNB_HOME_PANEL_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), MNB_TYPE_HOME_PANEL, MnbHomePanelClass))
#define MNB_IS_HOME_PANEL(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_HOME_PANEL))
#define MNB_IS_HOME_PANEL_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), MNB_TYPE_HOME_PANEL))
#define MNB_HOME_PANEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_HOME_PANEL, MnbHomePanelClass))

typedef struct _MnbHomePanel MnbHomePanel;
typedef struct _MnbHomePanelClass MnbHomePanelClass;
typedef struct _MnbHomePanelPrivate MnbHomePanelPrivate;

struct _MnbHomePanel
{
  MxTable parent;

  MnbHomePanelPrivate *priv;
};

struct _MnbHomePanelClass
{
  MxTableClass parent_class;
};

GType mnb_home_panel_get_type (void);
ClutterActor *mnb_home_panel_new (MplPanelClient *client);

G_END_DECLS

#endif
