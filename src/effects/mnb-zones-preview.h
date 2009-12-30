/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Chris Lord <chris@linux.intel.com>
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

#ifndef _MNB_ZONES_PREVIEW_H
#define _MNB_ZONES_PREVIEW_H

#include <glib-object.h>
#include <mutter-plugin.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MNB_TYPE_ZONES_PREVIEW mnb_zones_preview_get_type()

#define MNB_ZONES_PREVIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MNB_TYPE_ZONES_PREVIEW, MnbZonesPreview))

#define MNB_ZONES_PREVIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MNB_TYPE_ZONES_PREVIEW, MnbZonesPreviewClass))

#define MNB_IS_ZONES_PREVIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MNB_TYPE_ZONES_PREVIEW))

#define MNB_IS_ZONES_PREVIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MNB_TYPE_ZONES_PREVIEW))

#define MNB_ZONES_PREVIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MNB_TYPE_ZONES_PREVIEW, MnbZonesPreviewClass))

typedef struct _MnbZonesPreview MnbZonesPreview;
typedef struct _MnbZonesPreviewClass MnbZonesPreviewClass;
typedef struct _MnbZonesPreviewPrivate MnbZonesPreviewPrivate;

struct _MnbZonesPreview
{
  MxWidget parent;

  MnbZonesPreviewPrivate *priv;
};

struct _MnbZonesPreviewClass
{
  MxWidgetClass parent_class;

  void (* switch_completed) (MnbZonesPreview *preview);
};

GType mnb_zones_preview_get_type (void) G_GNUC_CONST;

ClutterActor *mnb_zones_preview_new (void);

void mnb_zones_preview_add_window (MnbZonesPreview *preview,
                                   MutterWindow    *window);

void mnb_zones_preview_change_workspace (MnbZonesPreview *preview,
                                         gint             workspace);

void mnb_zones_preview_set_n_workspaces (MnbZonesPreview *preview,
                                         gint             workspace);

void mnb_zones_preview_clear (MnbZonesPreview *preview);

G_END_DECLS

#endif /* _MNB_ZONES_PREVIEW_H */
