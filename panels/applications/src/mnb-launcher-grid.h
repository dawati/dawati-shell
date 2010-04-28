/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Robert Staudinger <robertx.staudinger@intel.com>
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

#ifndef MNB_LAUNCHER_GRID_H
#define MNB_LAUNCHER_GRID_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MNB_TYPE_LAUNCHER_GRID mnb_launcher_grid_get_type()

#define MNB_LAUNCHER_GRID(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MNB_TYPE_LAUNCHER_GRID, MnbLauncherGrid))

#define MNB_LAUNCHER_GRID_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MNB_TYPE_LAUNCHER_GRID, MnbLauncherGridClass))

#define MNB_IS_LAUNCHER_GRID(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MNB_TYPE_LAUNCHER_GRID))

#define MNB_IS_LAUNCHER_GRID_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MNB_TYPE_LAUNCHER_GRID))

#define MNB_LAUNCHER_GRID_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MNB_TYPE_LAUNCHER_GRID, MnbLauncherGridClass))

typedef struct {
  /*< private >*/
  MxGrid parent;
} MnbLauncherGrid;

typedef struct {
  MxGridClass parent_class;
} MnbLauncherGridClass;

GType mnb_launcher_grid_get_type (void);

MxWidget  * mnb_launcher_grid_new         (void);

MxWidget  * mnb_launcher_grid_keynav        (MnbLauncherGrid  *self,
                                             guint             keyval);
MxWidget *  mnb_launcher_grid_keynav_up     (MnbLauncherGrid  *self);
MxWidget *  mnb_launcher_grid_keynav_down   (MnbLauncherGrid  *self);
MxWidget *  mnb_launcher_grid_keynav_first  (MnbLauncherGrid  *self);
void        mnb_launcher_grid_keynav_out    (MnbLauncherGrid  *self);

MxWidget *  mnb_launcher_grid_find_widget_by_point        (MnbLauncherGrid  *self,
                                                           gfloat            x,
                                                           gfloat            y);

MxWidget  * mnb_launcher_grid_find_widget_by_pseudo_class (MnbLauncherGrid  *self,
                                                           const gchar      *pseudo_class);

gboolean   mnb_launcher_grid_get_x_expand_children (MnbLauncherGrid *self);

void       mnb_launcher_grid_set_x_expand_children (MnbLauncherGrid *self,
                                                    gboolean         value);

#endif /* MNB_LAUNCHER_GRID_H */

