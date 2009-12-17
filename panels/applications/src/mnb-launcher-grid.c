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

#include "mnb-launcher-grid.h"

G_DEFINE_TYPE (MnbLauncherGrid, mnb_launcher_grid, MX_TYPE_GRID)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_LAUNCHER_GRID, MnbLauncherGridPrivate))

typedef struct _MnbLauncherGridPrivate MnbLauncherGridPrivate;

struct _MnbLauncherGridPrivate
{
  int placeholder;
};

static void
mnb_launcher_grid_class_init (MnbLauncherGridClass *klass)
{
  g_type_class_add_private (klass, sizeof (MnbLauncherGridPrivate));
}

static void
mnb_launcher_grid_init (MnbLauncherGrid *self)
{}

MxWidget *
mnb_launcher_grid_new (void)
{
  return g_object_new (MNB_TYPE_LAUNCHER_GRID, NULL);
}

typedef struct
{
  const gchar *pseudo_class;
  MxWidget    *widget;
} find_widget_by_pseudo_class_data;

static void
_find_widget_by_pseudo_class_cb (ClutterActor                      *actor,
                                 find_widget_by_pseudo_class_data  *data)
{
  const gchar *pseudo_class;

  if (!CLUTTER_ACTOR_IS_MAPPED (actor))
    return;

  if (!MX_IS_STYLABLE (actor))
    return;

  pseudo_class = mx_widget_get_style_pseudo_class (MX_WIDGET (actor));
  if (0 == g_strcmp0 (data->pseudo_class, pseudo_class))
    data->widget = MX_WIDGET (actor);
}

MxWidget *
mnb_launcher_grid_find_widget_by_pseudo_class (MnbLauncherGrid  *grid,
                                               const gchar      *pseudo_class)
{
  find_widget_by_pseudo_class_data data;

  data.pseudo_class = pseudo_class;
  data.widget = NULL;

  clutter_container_foreach (CLUTTER_CONTAINER (grid),
                             (ClutterCallback) _find_widget_by_pseudo_class_cb,
                             &data);

  return data.widget;
}

typedef struct {
  gfloat  x;
  gfloat  y;
  MxWidget  *widget;
} find_widget_by_point_data_t;

static void
_find_widget_by_point_cb (ClutterActor                *actor,
                          find_widget_by_point_data_t *data)
{
  gfloat left = clutter_actor_get_x (actor);
  gfloat top = clutter_actor_get_y (actor);
  gfloat right = left + clutter_actor_get_width (actor);
  gfloat bottom = top + clutter_actor_get_height (actor);

  if (CLUTTER_ACTOR_IS_MAPPED (actor) &&
      left <= data->x &&
      top <= data->y &&
      right >= data->x &&
      bottom >= data->y)
    {
      data->widget = MX_WIDGET (actor);
    }
}

MxWidget *
mnb_launcher_grid_find_widget_by_point (MnbLauncherGrid *self,
                                        gfloat           x,
                                        gfloat           y)
{
  find_widget_by_point_data_t data;

  data.x = x;
  data.y = y;
  data.widget = NULL;

  clutter_container_foreach (CLUTTER_CONTAINER (self),
                             (ClutterCallback) _find_widget_by_point_cb,
                             &data);

  return data.widget;
}

MxWidget *
mnb_launcher_grid_keynav_up (MnbLauncherGrid *self)
{
  MxWidget    *old, *new;
  gfloat       x, y;

  old = mnb_launcher_grid_find_widget_by_pseudo_class (self, "hover");
  if (old == NULL)
    return NULL;

  x = clutter_actor_get_x (CLUTTER_ACTOR (old)) +
      clutter_actor_get_width (CLUTTER_ACTOR (old)) / 2;

  y = clutter_actor_get_y (CLUTTER_ACTOR (old)) -
      mx_grid_get_row_spacing (MX_GRID (self)) -
      clutter_actor_get_height (CLUTTER_ACTOR (old)) / 2;

  new = mnb_launcher_grid_find_widget_by_point (self, x, y);
  if (new)
    {
      mx_widget_set_style_pseudo_class (old, NULL);
      mx_widget_set_style_pseudo_class (new, "hover");
      return new;
    }

  return NULL;
}

static MxWidget *
mnb_launcher_grid_keynav_right (MnbLauncherGrid *self)
{
  MxWidget    *old, *new;
  gfloat       x, y;

  old = mnb_launcher_grid_find_widget_by_pseudo_class (self, "hover");
  if (old == NULL)
    return NULL;

  x = clutter_actor_get_x (CLUTTER_ACTOR (old)) +
      mx_grid_get_column_spacing (MX_GRID (self)) +
      clutter_actor_get_width (CLUTTER_ACTOR (old)) * 1.5;

  y = clutter_actor_get_y (CLUTTER_ACTOR (old)) +
      clutter_actor_get_height (CLUTTER_ACTOR (old)) / 2;

  new = mnb_launcher_grid_find_widget_by_point (self, x, y);
  if (new)
    {
      mx_widget_set_style_pseudo_class (old, NULL);
      mx_widget_set_style_pseudo_class (new, "hover");
      return new;
    }

  return NULL;
}

MxWidget *
mnb_launcher_grid_keynav_down (MnbLauncherGrid *self)
{
  MxWidget    *old, *new;
  gfloat       x, y;

  old = mnb_launcher_grid_find_widget_by_pseudo_class (self, "hover");
  if (old == NULL)
    return NULL;

  x = clutter_actor_get_x (CLUTTER_ACTOR (old)) +
      clutter_actor_get_width (CLUTTER_ACTOR (old)) / 2;

  y = clutter_actor_get_y (CLUTTER_ACTOR (old)) +
      mx_grid_get_row_spacing (MX_GRID (self)) +
      clutter_actor_get_height (CLUTTER_ACTOR (old)) * 1.5;

  new = mnb_launcher_grid_find_widget_by_point (self, x, y);
  if (new)
    {
      mx_widget_set_style_pseudo_class (old, NULL);
      mx_widget_set_style_pseudo_class (new, "hover");
      return new;
    }

  return NULL;
}

static MxWidget *
mnb_launcher_grid_keynav_left (MnbLauncherGrid *self)
{
  MxWidget    *old, *new;
  gfloat       x, y;

  old = mnb_launcher_grid_find_widget_by_pseudo_class (self, "hover");
  if (old == NULL)
    return NULL;

  x = clutter_actor_get_x (CLUTTER_ACTOR (old)) -
      mx_grid_get_column_spacing (MX_GRID (self)) -
      clutter_actor_get_width (CLUTTER_ACTOR (old)) / 2;

  y = clutter_actor_get_y (CLUTTER_ACTOR (old)) +
      clutter_actor_get_height (CLUTTER_ACTOR (old)) / 2;

  new = mnb_launcher_grid_find_widget_by_point (self, x, y);
  if (new)
    {
      mx_widget_set_style_pseudo_class (old, NULL);
      mx_widget_set_style_pseudo_class (new, "hover");
      return new;
    }

  return NULL;
}

static MxWidget *
mnb_launcher_grid_keynav_wrap_up (MnbLauncherGrid *self)
{
  MxWidget    *old, *new;
  MxPadding    padding;
  gfloat       x, y;

  old = mnb_launcher_grid_find_widget_by_pseudo_class (self, "hover");
  if (old == NULL)
    return NULL;

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  x = clutter_actor_get_width (CLUTTER_ACTOR (self)) -
      padding.right -
      clutter_actor_get_width (CLUTTER_ACTOR (old)) / 2;

  y = clutter_actor_get_y (CLUTTER_ACTOR (old)) -
      mx_grid_get_row_spacing (MX_GRID (self)) -
      clutter_actor_get_height (CLUTTER_ACTOR (old)) / 2;

  new = mnb_launcher_grid_find_widget_by_point (self, x, y);
  if (new)
    {
      mx_widget_set_style_pseudo_class (old, NULL);
      mx_widget_set_style_pseudo_class (new, "hover");
      return new;
    }

  return NULL;
}

static MxWidget *
mnb_launcher_grid_keynav_wrap_down (MnbLauncherGrid *self)
{
  MxWidget    *old, *new;
  MxPadding    padding;
  gfloat       x, y;

  old = mnb_launcher_grid_find_widget_by_pseudo_class (self, "hover");
  if (old == NULL)
    return NULL;

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  x = padding.left +
      clutter_actor_get_width (CLUTTER_ACTOR (old)) / 2;

  y = clutter_actor_get_y (CLUTTER_ACTOR (old)) +
      mx_grid_get_row_spacing (MX_GRID (self)) +
      clutter_actor_get_height (CLUTTER_ACTOR (old)) * 1.5;

  new = mnb_launcher_grid_find_widget_by_point (self, x, y);
  if (new)
    {
      mx_widget_set_style_pseudo_class (old, NULL);
      mx_widget_set_style_pseudo_class (new, "hover");
      return new;
    }

  return NULL;
}

MxWidget *
mnb_launcher_grid_keynav_first (MnbLauncherGrid *self)
{
  MxWidget    *widget;
  MxPadding    padding;
  gfloat       x, y;

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  x = padding.left + 1;
  y = padding.top + 1;

  widget = mnb_launcher_grid_find_widget_by_point (self, x, y);
  if (widget)
    {
      mx_widget_set_style_pseudo_class (widget, "hover");
      return widget;
    }

  return NULL;
}

void
mnb_launcher_grid_keynav_out (MnbLauncherGrid *self)
{
  MxWidget  *widget;

  widget = mnb_launcher_grid_find_widget_by_pseudo_class (self, "hover");
  if (widget)
    mx_widget_set_style_pseudo_class (widget, NULL);
}

MxWidget *
mnb_launcher_grid_keynav (MnbLauncherGrid *self,
                          guint            keyval)
{
  MxWidget *widget;

  widget = NULL;
  switch (keyval)
    {
      case CLUTTER_Return:
        widget = mnb_launcher_grid_find_widget_by_pseudo_class (self, "hover");
        break;
      case CLUTTER_Left:
        widget = mnb_launcher_grid_keynav_left (self);
        if (!widget)
          widget = mnb_launcher_grid_keynav_wrap_up (self);
        break;
      case CLUTTER_Up:
        widget = mnb_launcher_grid_keynav_up (self);
        break;
      case CLUTTER_Right:
        widget = mnb_launcher_grid_keynav_right (self);
        if (!widget)
          widget = mnb_launcher_grid_keynav_wrap_down (self);
        break;
      case CLUTTER_Down:
        widget = mnb_launcher_grid_keynav_down (self);
        break;
    }

  return widget;
}

