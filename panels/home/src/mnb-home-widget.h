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

#ifndef __MNB_HOME_WIDGET_H__
#define __MNB_HOME_WIDGET_H__

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MNB_TYPE_HOME_WIDGET	(mnb_home_widget_get_type ())
#define MNB_HOME_WIDGET(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_HOME_WIDGET, MnbHomeWidget))
#define MNB_HOME_WIDGET_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), MNB_TYPE_HOME_WIDGET, MnbHomeWidgetClass))
#define MNB_IS_HOME_WIDGET(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_HOME_WIDGET))
#define MNB_IS_HOME_WIDGET_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), MNB_TYPE_HOME_WIDGET))
#define MNB_HOME_WIDGET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_HOME_WIDGET, MnbHomeWidgetClass))

typedef struct _MnbHomeWidget MnbHomeWidget;
typedef struct _MnbHomeWidgetClass MnbHomeWidgetClass;
typedef struct _MnbHomeWidgetPrivate MnbHomeWidgetPrivate;

struct _MnbHomeWidget
{
  MxFrame parent;

  MnbHomeWidgetPrivate *priv;
};

struct _MnbHomeWidgetClass
{
  MxFrameClass parent_class;
};

GType mnb_home_widget_get_type (void);
ClutterActor *mnb_home_widget_new (const char *object_path);
void mnb_home_widget_set_edit_mode (MnbHomeWidget *self, gboolean edit_mode);

G_END_DECLS

#endif
