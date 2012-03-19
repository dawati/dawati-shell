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

#ifndef __MNB_HOME_WIDGET_PREVIEW_H__
#define __MNB_HOME_WIDGET_PREVIEW_H__

#include <glib-object.h>

#include <mx/mx.h>

G_BEGIN_DECLS

#define MNB_TYPE_HOME_WIDGET_PREVIEW	(mnb_home_widget_preview_get_type ())
#define MNB_HOME_WIDGET_PREVIEW(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_HOME_WIDGET_PREVIEW, MnbHomeWidgetPreview))
#define MNB_HOME_WIDGET_PREVIEW_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), MNB_TYPE_HOME_WIDGET_PREVIEW, MnbHomeWidgetPreviewClass))
#define MNB_IS_HOME_WIDGET_PREVIEW(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_HOME_WIDGET_PREVIEW))
#define MNB_IS_HOME_WIDGET_PREVIEW_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), MNB_TYPE_HOME_WIDGET_PREVIEW))
#define MNB_HOME_WIDGET_PREVIEW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_HOME_WIDGET_PREVIEW, MnbHomeWidgetPreviewClass))

typedef struct _MnbHomeWidgetPreview MnbHomeWidgetPreview;
typedef struct _MnbHomeWidgetPreviewClass MnbHomeWidgetPreviewClass;
typedef struct _MnbHomeWidgetPreviewPrivate MnbHomeWidgetPreviewPrivate;

struct _MnbHomeWidgetPreview
{
  MxTable parent;

  MnbHomeWidgetPreviewPrivate *priv;
};

struct _MnbHomeWidgetPreviewClass
{
  MxTableClass parent_class;
};

GType mnb_home_widget_preview_get_type (void);
const char *mnb_home_widget_preview_get_module (MnbHomeWidgetPreview *self);

G_END_DECLS

#endif
