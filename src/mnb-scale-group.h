/*
 * Authored Tomas Frydrych <tf@linux.intel.com>
 *
 * Copyright (C) 2008 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __MNB_SCALE_GROUP_H__
#define __MNB_SCALE_GROUP_H__

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MNB_TYPE_SCALE_GROUP mnb_scale_group_get_type()

#define MNB_SCALE_GROUP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MNB_TYPE_SCALE_GROUP, MnbScaleGroup))

#define MNB_SCALE_GROUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MNB_TYPE_SCALE_GROUP, MnbScaleGroupClass))

#define MNB_IS_SCALE_GROUP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MNB_TYPE_SCALE_GROUP))

#define MNB_IS_SCALE_GROUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MNB_TYPE_SCALE_GROUP))

#define MNB_SCALE_GROUP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MNB_TYPE_SCALE_GROUP, MnbScaleGroupClass))

typedef struct _MnbScaleGroup        MnbScaleGroup;
typedef struct _MnbScaleGroupClass   MnbScaleGroupClass;

struct _MnbScaleGroup
{
  ClutterGroup parent_instance;

  /*< private >*/
};

struct _MnbScaleGroupClass
{
  /*< private >*/
  ClutterGroupClass parent_class;
};

GType         mnb_scale_group_get_type         (void) G_GNUC_CONST;
ClutterActor *mnb_scale_group_new              (void);

G_END_DECLS

#endif /* __MNB_SCALE_GROUP_H__ */
