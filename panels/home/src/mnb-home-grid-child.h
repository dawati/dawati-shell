/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Copyright 2012 Intel Corporation.
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
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Lionel Landwerlin  <lionel.g.landwerlin@linux.intel.com>
 *
 */

#ifndef __MNB_HOME_GRID_CHILD_H__
#define __MNB_HOME_GRID_CHILD_H__

#include "mnb-home-grid.h"

G_BEGIN_DECLS

#define MNB_TYPE_HOME_GRID_CHILD                \
  (mnb_home_grid_child_get_type ())
#define MNB_HOME_GRID_CHILD(obj)                                        \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                   \
                               MNB_TYPE_HOME_GRID_CHILD,                \
                               MnbHomeGridChild))
#define MNB_IS_HOME_GRID_CHILD(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                   \
                               MNB_TYPE_HOME_GRID_CHILD))
#define MNB_HOME_GRID_CHILD_CLASS(klass)                                \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                                    \
                            MNB_TYPE_HOME_GRID_CHILD,                   \
                            MnbHomeGridChildClass))
#define MNB_IS_HOME_GRID_CHILD_CLASS(klass)                     \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                            \
                            MNB_TYPE_HOME_GRID_CHILD))
#define MNB_HOME_GRID_CHILD_GET_CLASS(obj)              \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                    \
                              MNB_TYPE_HOME_GRID_CHILD, \
                              MnbHomeGridChildClass))

/**
 * MnbHomeGridChild:
 *
 * The contents of this structure is private and should only be accessed using
 * the provided API.
 */
typedef struct _MnbHomeGridChild         MnbHomeGridChild;
typedef struct _MnbHomeGridChildClass    MnbHomeGridChildClass;

struct _MnbHomeGridChildClass
{
  ClutterChildMetaClass parent_class;

  /* padding for future expansion */
  void (*_padding_0) (void);
  void (*_padding_1) (void);
  void (*_padding_2) (void);
  void (*_padding_3) (void);
  void (*_padding_4) (void);
};

GType mnb_home_grid_child_get_type (void) G_GNUC_CONST;


gboolean mnb_home_grid_child_get_x_fill        (MnbHomeGrid  *grid,
                                                ClutterActor *child);
void     mnb_home_grid_child_set_x_fill        (MnbHomeGrid  *grid,
                                                ClutterActor *child,
                                                gboolean      fill);
gboolean mnb_home_grid_child_get_y_fill        (MnbHomeGrid  *grid,
                                                ClutterActor *child);
void     mnb_home_grid_child_set_y_fill        (MnbHomeGrid  *grid,
                                                ClutterActor *child,
                                                gboolean      fill);
gboolean mnb_home_grid_child_get_x_expand      (MnbHomeGrid  *grid,
                                                ClutterActor *child);
void     mnb_home_grid_child_set_x_expand      (MnbHomeGrid  *grid,
                                                ClutterActor *child,
                                                gboolean      expand);
gboolean mnb_home_grid_child_get_y_expand      (MnbHomeGrid  *grid,
                                                ClutterActor *child);
void     mnb_home_grid_child_set_y_expand      (MnbHomeGrid  *grid,
                                                ClutterActor *child,
                                                gboolean      expand);
MxAlign  mnb_home_grid_child_get_x_align       (MnbHomeGrid  *grid,
                                                ClutterActor *child);
void     mnb_home_grid_child_set_x_align       (MnbHomeGrid  *grid,
                                                ClutterActor *child,
                                                MxAlign       align);
MxAlign  mnb_home_grid_child_get_y_align       (MnbHomeGrid  *grid,
                                                ClutterActor *child);
void     mnb_home_grid_child_set_y_align       (MnbHomeGrid  *grid,
                                                ClutterActor *child,
                                                MxAlign       align);

G_END_DECLS

#endif /* __MYZONE_GRID_CHILD_H__ */
