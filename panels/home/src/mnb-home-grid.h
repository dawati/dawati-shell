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

#ifndef __MNB_HOME_GRID_H__
#define __MNB_HOME_GRID_H__

#include <glib-object.h>

#include <mx/mx.h>

G_BEGIN_DECLS

#define MNB_TYPE_HOME_GRID (mnb_home_grid_get_type())

#define MNB_HOME_GRID(obj)                              \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                   \
                               MNB_TYPE_HOME_GRID,      \
                               MnbHomeGrid))

#define MNB_HOME_GRID_CLASS(klass)              \
  (G_TYPE_CHECK_CLASS_CAST ((klass),            \
                            MNB_TYPE_HOME_GRID, \
                            MnbHomeGridClass))

#define MNB_IS_HOME_GRID(obj)                           \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               MNB_TYPE_HOME_GRID))

#define MNB_IS_HOME_GRID_CLASS(klass)                   \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                    \
                            MNB_TYPE_HOME_GRID))

#define MNB_HOME_GRID_GET_CLASS(obj)                    \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                    \
                              MNB_TYPE_HOME_GRID,       \
                              MnbHomeGridClass))

typedef struct _MnbHomeGrid MnbHomeGrid;
typedef struct _MnbHomeGridClass MnbHomeGridClass;
typedef struct _MnbHomeGridPrivate MnbHomeGridPrivate;

struct _MnbHomeGrid
{
  MxWidget parent;

  MnbHomeGridPrivate *priv;
};

struct _MnbHomeGridClass
{
  MxWidgetClass parent_class;
};

GType mnb_home_grid_get_type (void) G_GNUC_CONST;

ClutterActor *mnb_home_grid_new (void);

void mnb_home_grid_insert_actor (MnbHomeGrid  *table,
                               ClutterActor *actor,
                               gint          col,
                               gint          row);

void     mnb_home_grid_set_edit_mode (MnbHomeGrid *self, gboolean value);
gboolean mnb_home_grid_get_edit_mode (MnbHomeGrid *self);

void     mnb_home_grid_set_grid_size (MnbHomeGrid *grid, guint cols, guint rows);
void     mnb_home_grid_get_grid_size (MnbHomeGrid *grid, guint *cols, guint *rows);


G_END_DECLS

#endif /* __MNB_HOME_GRID_H__ */
