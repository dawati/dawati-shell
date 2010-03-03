/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Rob bradford <rob@linux.intel.com>
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

#ifndef _MPS_GEOTAG_PANE
#define _MPS_GEOTAG_PANE

#include <mx/mx.h>

G_BEGIN_DECLS

#define MPS_TYPE_GEOTAG_PANE mps_geotag_pane_get_type()

#define MPS_GEOTAG_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPS_TYPE_GEOTAG_PANE, MpsGeotagPane))

#define MPS_GEOTAG_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPS_TYPE_GEOTAG_PANE, MpsGeotagPaneClass))

#define MPS_IS_GEOTAG_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPS_TYPE_GEOTAG_PANE))

#define MPS_IS_GEOTAG_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPS_TYPE_GEOTAG_PANE))

#define MPS_GEOTAG_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPS_TYPE_GEOTAG_PANE, MpsGeotagPaneClass))

typedef struct {
  MxTable parent;
} MpsGeotagPane;

typedef struct {
  MxTableClass parent_class;
} MpsGeotagPaneClass;

GType mps_geotag_pane_get_type (void);

ClutterActor *mps_geotag_pane_new (void);

G_END_DECLS

#endif /* _MPS_GEOTAG_PANE */
