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

#ifndef __NUTTER_SCALE_GROUP_H__
#define __NUTTER_SCALE_GROUP_H__

#include <glib-object.h>
#include <clutter/clutter-types.h>
#include <clutter/clutter-group.h>

G_BEGIN_DECLS

#define NUTTER_TYPE_SCALE_GROUP nutter_scale_group_get_type()

#define NUTTER_SCALE_GROUP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  NUTTER_TYPE_SCALE_GROUP, NutterScaleGroup))

#define NUTTER_SCALE_GROUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  NUTTER_TYPE_SCALE_GROUP, NutterScaleGroupClass))

#define NUTTER_IS_SCALE_GROUP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  NUTTER_TYPE_SCALE_GROUP))

#define NUTTER_IS_SCALE_GROUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  NUTTER_TYPE_SCALE_GROUP))

#define NUTTER_SCALE_GROUP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  NUTTER_TYPE_SCALE_GROUP, NutterScaleGroupClass))

typedef struct _NutterScaleGroup        NutterScaleGroup;
typedef struct _NutterScaleGroupClass   NutterScaleGroupClass;

struct _NutterScaleGroup
{
  ClutterGroup parent_instance;

  /*< private >*/
};

struct _NutterScaleGroupClass
{
  /*< private >*/
  ClutterGroupClass parent_class;
};

GType         nutter_scale_group_get_type         (void) G_GNUC_CONST;
ClutterActor *nutter_scale_group_new              (void);

G_END_DECLS

#endif /* __NUTTER_SCALE_GROUP_H__ */
