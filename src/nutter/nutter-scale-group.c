/*
 * Authored Tomas Frydrych <tf@linux.intel.com>
 *
 * Copyright (C) 2008 Intel Corporation
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>

#include "nutter-scale-group.h"

#include <clutter/clutter-container.h>

G_DEFINE_TYPE (NutterScaleGroup, nutter_scale_group, CLUTTER_TYPE_GROUP);

static void
nutter_scale_group_get_preferred_width (ClutterActor *self,
					ClutterUnit   for_height,
					ClutterUnit  *min_width_p,
					ClutterUnit  *natural_width_p)
{
  gdouble scale_x, scale_y;

  CLUTTER_ACTOR_CLASS (nutter_scale_group_parent_class)->get_preferred_width (
							      self,
							      for_height,
							      min_width_p,
							      natural_width_p);

  clutter_actor_get_scale (self, &scale_x, &scale_y);

  if (min_width_p)
    *min_width_p = (ClutterUnit)((gdouble)*min_width_p * scale_x);

  if (natural_width_p)
    *natural_width_p = (ClutterUnit)((gdouble)*natural_width_p * scale_x);
}

static void
nutter_scale_group_get_preferred_height (ClutterActor *self,
					 ClutterUnit   for_width,
					 ClutterUnit  *min_height_p,
					 ClutterUnit  *natural_height_p)
{
  gdouble scale_x, scale_y;

  CLUTTER_ACTOR_CLASS (nutter_scale_group_parent_class)->get_preferred_height (
							      self,
							      for_width,
							      min_height_p,
							      natural_height_p);

  clutter_actor_get_scale (self, &scale_x, &scale_y);

  if (min_height_p)
    *min_height_p = (ClutterUnit)((gdouble)*min_height_p * scale_y);

  if (natural_height_p)
    *natural_height_p = (ClutterUnit)((gdouble)*natural_height_p * scale_y);
}

static void
nutter_scale_group_class_init (NutterScaleGroupClass *klass)
{
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);

  actor_class->get_preferred_width  = nutter_scale_group_get_preferred_width;
  actor_class->get_preferred_height = nutter_scale_group_get_preferred_height;
}

static void
nutter_scale_group_on_scale_change (GObject     *object,
				    GParamSpec  *param_spec,
				    gpointer     data)
{
  clutter_actor_queue_relayout (CLUTTER_ACTOR (object));
}

static void
nutter_scale_group_init (NutterScaleGroup *self)
{
  g_signal_connect (self,
                    "notify::scale-x",
                    G_CALLBACK(nutter_scale_group_on_scale_change), NULL);

  g_signal_connect (self,
                    "notify::scale-y",
                    G_CALLBACK(nutter_scale_group_on_scale_change), NULL);
}

ClutterActor *
nutter_scale_group_new (void)
{
  return g_object_new (NUTTER_TYPE_SCALE_GROUP, NULL);
}

