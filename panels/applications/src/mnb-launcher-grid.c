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

enum
{
  PROP_0,

  PROP_X_EXPAND_CHILDREN
};

typedef struct
{
  gboolean x_expand_children;
} MnbLauncherGridPrivate;

static void
_set_width_cb (ClutterActor *actor,
               gfloat const *width)
{
  clutter_actor_set_width (actor, *width);
}

static gboolean
_allocation_changed_idle_cb (MnbLauncherGrid *self)
{
  MnbLauncherGridPrivate *priv = GET_PRIVATE (self);

  if (priv && priv->x_expand_children)
    {
      MxPadding padding;
      gfloat width;

      mx_widget_get_padding (MX_WIDGET (self), &padding);

      width = clutter_actor_get_width ((ClutterActor *) self) -
              padding.left - padding.right;

      clutter_container_foreach (CLUTTER_CONTAINER (self),
                                 (ClutterCallback) _set_width_cb,
                                 (gpointer) &width);
    }

  return FALSE;
}

static void
_allocation_changed_cb (MnbLauncherGrid         *self,
                        ClutterActorBox         *box,
                        ClutterAllocationFlags   flags,
                        gpointer                 data)
{
  g_idle_add_full (G_PRIORITY_HIGH_IDLE,
                   (GSourceFunc) _allocation_changed_idle_cb,
                   self,
                   NULL);
}

static void
_get_property (GObject    *object,
               guint       property_id,
               GValue     *value,
               GParamSpec *pspec)
{
  switch (property_id)
  {
  case PROP_X_EXPAND_CHILDREN:
    g_value_set_boolean (value,
                         mnb_launcher_grid_get_x_expand_children (
                            MNB_LAUNCHER_GRID (object)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_set_property (GObject      *object,
               guint         property_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  switch (property_id)
  {
  case PROP_X_EXPAND_CHILDREN:
    mnb_launcher_grid_set_x_expand_children (MNB_LAUNCHER_GRID (object),
                                             g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_launcher_grid_class_init (MnbLauncherGridClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbLauncherGridPrivate));

  object_class->get_property = _get_property;
  object_class->set_property = _set_property;

  /* Properties */

  g_object_class_install_property (object_class,
                                   PROP_X_EXPAND_CHILDREN,
                                   g_param_spec_boolean ("x-expand-children",
                                                         "X-expand children",
                                                         "Wheter to assign full width to the children",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
}

static void
mnb_launcher_grid_init (MnbLauncherGrid *self)
{
  g_signal_connect (self, "allocation-changed",
                    G_CALLBACK (_allocation_changed_cb), NULL);
}

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

  pseudo_class = mx_stylable_get_style_pseudo_class (MX_STYLABLE (actor));
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
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (old), NULL);
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (new), "hover");
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
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (old), NULL);
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (new), "hover");
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
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (old), NULL);
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (new), "hover");
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
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (old), NULL);
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (new), "hover");
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
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (old), NULL);
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (new), "hover");
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
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (old), NULL);
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (new), "hover");
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
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (widget), "hover");
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
    mx_stylable_set_style_pseudo_class (MX_STYLABLE (widget), NULL);
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

gboolean
mnb_launcher_grid_get_x_expand_children (MnbLauncherGrid *self)
{
  MnbLauncherGridPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MNB_IS_LAUNCHER_GRID (self), FALSE);

  return priv->x_expand_children;
}

void
mnb_launcher_grid_set_x_expand_children (MnbLauncherGrid *self,
                                         gboolean         value)
{
  MnbLauncherGridPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MNB_IS_LAUNCHER_GRID (self));

  if (value != priv->x_expand_children)
    {
      priv->x_expand_children = value;
      g_object_notify ((G_OBJECT (self)), "x-expand-children");
    }
}

