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

#ifndef __MNB_HOME_GRID_PRIVATE_H__
#define __MNB_HOME_GRID_PRIVATE_H__

#include <clutter/clutter.h>

struct _MnbHomeGridChild
{
  ClutterChildMeta parent_instance;

  gint col;
  gint row;
  gint width;
  gint height;
  gdouble x_align;
  gdouble y_align;
  guint x_expand : 1;
  guint y_expand : 1;
  guint x_fill : 1;
  guint y_fill : 1;
};


#endif /* __MNB_HOME_GRID_PRIVATE_H__ */
