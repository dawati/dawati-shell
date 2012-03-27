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

#include "mnb-home-grid-child.h"
#include "mnb-home-grid-private.h"

/*
 * ClutterChildMeta Implementation
 */

/**
 * SECTION:mnb_home-grid-child
 * @short_description: The child property store for #MnbHomeGrid
 *
 * The #ClutterChildMeta implementation for the #MnbHomeGrid container widget.
 *
 */

enum {
  CHILD_PROP_0,

  CHILD_PROP_X_EXPAND,
  CHILD_PROP_Y_EXPAND,
  CHILD_PROP_X_ALIGN,
  CHILD_PROP_Y_ALIGN,
  CHILD_PROP_X_FILL,
  CHILD_PROP_Y_FILL,
};

G_DEFINE_TYPE (MnbHomeGridChild, mnb_home_grid_child, CLUTTER_TYPE_CHILD_META);

static void
grid_child_set_property (GObject      *gobject,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  MnbHomeGridChild *child = MNB_HOME_GRID_CHILD (gobject);
  MnbHomeGrid *grid = MNB_HOME_GRID (CLUTTER_CHILD_META(gobject)->container);

  switch (prop_id)
    {
    case CHILD_PROP_X_EXPAND:
      child->x_expand = g_value_get_boolean (value);
      clutter_actor_queue_relayout (CLUTTER_ACTOR (grid));
      break;
    case CHILD_PROP_Y_EXPAND:
      child->y_expand = g_value_get_boolean (value);
      clutter_actor_queue_relayout (CLUTTER_ACTOR (grid));
      break;
    case CHILD_PROP_X_ALIGN:
      switch (g_value_get_enum (value))
        {
        case MX_ALIGN_START:
          child->x_align = 0.0;
          break;
        case MX_ALIGN_MIDDLE:
          child->x_align = 0.5;
          break;
        case MX_ALIGN_END:
          child->x_align = 1.0;
          break;
        }
      clutter_actor_queue_relayout (CLUTTER_ACTOR (grid));
      break;
    case CHILD_PROP_Y_ALIGN:
      switch (g_value_get_enum (value))
        {
        case MX_ALIGN_START:
          child->y_align = 0.0;
          break;
        case MX_ALIGN_MIDDLE:
          child->y_align = 0.5;
          break;
        case MX_ALIGN_END:
          child->y_align = 1.0;
          break;
        }
      clutter_actor_queue_relayout (CLUTTER_ACTOR (grid));
      break;
    case CHILD_PROP_X_FILL:
      child->x_fill = g_value_get_boolean (value);
      clutter_actor_queue_relayout (CLUTTER_ACTOR (grid));
      break;
    case CHILD_PROP_Y_FILL:
      child->y_fill = g_value_get_boolean (value);
      clutter_actor_queue_relayout (CLUTTER_ACTOR (grid));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
grid_child_get_property (GObject    *gobject,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  MnbHomeGridChild *child = MNB_HOME_GRID_CHILD (gobject);

  switch (prop_id)
    {
    case CHILD_PROP_X_EXPAND:
      g_value_set_boolean (value, child->x_expand);
      break;
    case CHILD_PROP_Y_EXPAND:
      g_value_set_boolean (value, child->y_expand);
      break;
    case CHILD_PROP_X_ALIGN:
      if (child->x_align < 1.0/3.0)
        g_value_set_enum (value, MX_ALIGN_START);
      else if (child->x_align > 2.0/3.0)
        g_value_set_enum (value, MX_ALIGN_END);
      else
        g_value_set_enum (value, MX_ALIGN_MIDDLE);
      break;
    case CHILD_PROP_Y_ALIGN:
      if (child->y_align < 1.0/3.0)
        g_value_set_enum (value, MX_ALIGN_START);
      else if (child->y_align > 2.0/3.0)
        g_value_set_enum (value, MX_ALIGN_END);
      else
        g_value_set_enum (value, MX_ALIGN_MIDDLE);
      break;
    case CHILD_PROP_X_FILL:
      g_value_set_boolean (value, child->x_fill);
      break;
    case CHILD_PROP_Y_FILL:
      g_value_set_boolean (value, child->y_fill);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_home_grid_child_class_init (MnbHomeGridChildClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  gobject_class->set_property = grid_child_set_property;
  gobject_class->get_property = grid_child_get_property;


  pspec = g_param_spec_boolean ("x-expand",
                                "X Expand",
                                "Whether the child should receive priority "
                                "when the container is allocating spare space "
                                "on the horizontal axis",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (gobject_class, CHILD_PROP_X_EXPAND, pspec);

  pspec = g_param_spec_boolean ("y-expand",
                                "Y Expand",
                                "Whether the child should receive priority "
                                "when the container is allocating spare space "
                                "on the vertical axis",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (gobject_class, CHILD_PROP_Y_EXPAND, pspec);

  pspec = g_param_spec_enum ("x-align",
                             "X Alignment",
                             "X alignment of the widget within the cell",
                             MX_TYPE_ALIGN,
                             MX_ALIGN_MIDDLE,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (gobject_class, CHILD_PROP_X_ALIGN, pspec);

  pspec = g_param_spec_enum ("y-align",
                             "Y Alignment",
                             "Y alignment of the widget within the cell",
                             MX_TYPE_ALIGN,
                             MX_ALIGN_MIDDLE,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (gobject_class, CHILD_PROP_Y_ALIGN, pspec);

  pspec = g_param_spec_boolean ("x-fill",
                                "X Fill",
                                "Whether the child should be allocated its "
                                "entire available space, or whether it should "
                                "be squashed and aligned.",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (gobject_class, CHILD_PROP_X_FILL, pspec);

  pspec = g_param_spec_boolean ("y-fill",
                                "Y Fill",
                                "Whether the child should be allocated its "
                                "entire available space, or whether it should "
                                "be squashed and aligned.",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (gobject_class, CHILD_PROP_Y_FILL, pspec);
}

static void
mnb_home_grid_child_init (MnbHomeGridChild *self)
{
  self->width = 1;
  self->height = 1;

  self->x_align = 0.5;
  self->y_align = 0.5;

  self->x_expand = TRUE;
  self->y_expand = TRUE;

  self->x_fill = TRUE;
  self->y_fill = TRUE;

}

static MnbHomeGridChild*
get_child_meta (MnbHomeGrid  *grid,
                ClutterActor *child)
{
  MnbHomeGridChild *meta;

  meta = (MnbHomeGridChild*) clutter_container_get_child_meta (CLUTTER_CONTAINER (grid), child);

  return meta;
}

/**
 * mnb_home_grid_child_get_x_fill:
 * @grid: A #MnbHomeGrid
 * @child: A #ClutterActor
 *
 * Get the x-fill state of the child
 *
 * Returns: #TRUE if the child is set to x-fill
 */
gboolean
mnb_home_grid_child_get_x_fill (MnbHomeGrid  *grid,
                                ClutterActor *child)
{
  MnbHomeGridChild *meta;

  g_return_val_if_fail (MNB_IS_HOME_GRID (grid), 0);
  g_return_val_if_fail (CLUTTER_IS_ACTOR (child), 0);

  meta = get_child_meta (grid, child);

  return meta->x_fill;
}

/**
 * mnb_home_grid_child_set_x_fill:
 * @grid: A #MnbHomeGrid
 * @child: A #ClutterActor
 * @fill: the fill state
 *
 * Set the fill state of the child on the x-axis. This will cause the child to
 * be allocated the maximum available space.
 *
 */
void
mnb_home_grid_child_set_x_fill (MnbHomeGrid  *grid,
                                ClutterActor *child,
                                gboolean      fill)
{
  MnbHomeGridChild *meta;

  g_return_if_fail (MNB_IS_HOME_GRID (grid));
  g_return_if_fail (CLUTTER_IS_ACTOR (child));

  meta = get_child_meta (grid, child);

  meta->x_fill = fill;

  clutter_actor_queue_relayout (child);
}


/**
 * mnb_home_grid_child_get_y_fill:
 * @grid: A #MnbHomeGrid
 * @child: A #ClutterActor
 *
 * Get the y-fill state of the child
 *
 * Returns: #TRUE if the child is set to y-fill
 */
gboolean
mnb_home_grid_child_get_y_fill (MnbHomeGrid  *grid,
                                ClutterActor *child)
{
  MnbHomeGridChild *meta;

  g_return_val_if_fail (MNB_IS_HOME_GRID (grid), 0);
  g_return_val_if_fail (CLUTTER_IS_ACTOR (child), 0);

  meta = get_child_meta (grid, child);

  return meta->y_fill;
}

/**
 * mnb_home_grid_child_set_y_fill:
 * @grid: A #MnbHomeGrid
 * @child: A #ClutterActor
 * @fill: the fill state
 *
 * Set the fill state of the child on the y-axis. This will cause the child to
 * be allocated the maximum available space.
 *
 */
void
mnb_home_grid_child_set_y_fill (MnbHomeGrid  *grid,
                                ClutterActor *child,
                                gboolean      fill)
{
  MnbHomeGridChild *meta;

  g_return_if_fail (MNB_IS_HOME_GRID (grid));
  g_return_if_fail (CLUTTER_IS_ACTOR (child));

  meta = get_child_meta (grid, child);

  meta->y_fill = fill;

  clutter_actor_queue_relayout (child);
}

/**
 * mnb_home_grid_child_get_x_expand:
 * @grid: A #MnbHomeGrid
 * @child: A #ClutterActor
 *
 * Get the x-expand property of the child
 *
 * Returns: #TRUE if the child is set to x-expand
 */
gboolean
mnb_home_grid_child_get_x_expand (MnbHomeGrid  *grid,
                                  ClutterActor *child)
{
  MnbHomeGridChild *meta;

  g_return_val_if_fail (MNB_IS_HOME_GRID (grid), 0);
  g_return_val_if_fail (CLUTTER_IS_ACTOR (child), 0);

  meta = get_child_meta (grid, child);

  return meta->x_expand;
}

/**
 * mnb_home_grid_child_set_x_expand:
 * @grid: A #MnbHomeGrid
 * @child: A #ClutterActor
 * @expand: the new value of the x expand child property
 *
 * Set x-expand on the child. This causes the column which the child
 * resides in to be allocated any extra space if the allocation of the grid is
 * larger than the preferred size.
 *
 */
void
mnb_home_grid_child_set_x_expand (MnbHomeGrid  *grid,
                                  ClutterActor *child,
                                  gboolean      expand)
{
  MnbHomeGridChild *meta;

  g_return_if_fail (MNB_IS_HOME_GRID (grid));
  g_return_if_fail (CLUTTER_IS_ACTOR (child));

  meta = get_child_meta (grid, child);

  meta->x_expand = expand;

  clutter_actor_queue_relayout (child);
}

/**
 * mnb_home_grid_child_set_y_expand:
 * @grid: A #MnbHomeGrid
 * @child: A #ClutterActor
 * @expand: the new value of the y-expand child property
 *
 * Set y-expand on the child. This causes the row which the child
 * resides in to be allocated any extra space if the allocation of the grid is
 * larger than the preferred size.
 *
 */
void
mnb_home_grid_child_set_y_expand (MnbHomeGrid  *grid,
                                  ClutterActor *child,
                                  gboolean      expand)
{
  MnbHomeGridChild *meta;

  g_return_if_fail (MNB_IS_HOME_GRID (grid));
  g_return_if_fail (CLUTTER_IS_ACTOR (child));

  meta = get_child_meta (grid, child);

  meta->y_expand = expand;

  clutter_actor_queue_relayout (child);
}

/**
 * mnb_home_grid_child_get_y_expand:
 * @grid: A #MnbHomeGrid
 * @child: A #ClutterActor
 *
 * Get the y-expand property of the child.
 *
 * Returns: #TRUE if the child is set to y-expand
 */
gboolean
mnb_home_grid_child_get_y_expand (MnbHomeGrid  *grid,
                                  ClutterActor *child)
{
  MnbHomeGridChild *meta;

  g_return_val_if_fail (MNB_IS_HOME_GRID (grid), 0);
  g_return_val_if_fail (CLUTTER_IS_ACTOR (child), 0);

  meta = get_child_meta (grid, child);

  return meta->y_expand;
}

/**
 * mnb_home_grid_child_get_x_align:
 * @grid: A #MnbHomeGrid
 * @child: A #ClutterActor
 *
 * Get the x-align value of the child
 *
 * Returns: An #MxAlign value
 */
MxAlign
mnb_home_grid_child_get_x_align (MnbHomeGrid  *grid,
                                 ClutterActor *child)
{
  MnbHomeGridChild *meta;

  g_return_val_if_fail (MNB_IS_HOME_GRID (grid), 0);
  g_return_val_if_fail (CLUTTER_IS_ACTOR (child), 0);

  meta = get_child_meta (grid, child);

  if (meta->x_align == 0.0)
    return MX_ALIGN_START;
  else if (meta->x_align == 1.0)
    return MX_ALIGN_END;
  else
    return MX_ALIGN_MIDDLE;

}

/**
 * mnb_home_grid_child_set_x_align:
 * @grid: A #MnbHomeGrid
 * @child: A #ClutterActor
 * @align: A #MxAlign value
 *
 * Set the alignment of the child within its cell. This will only have an effect
 * if the the x-fill property is FALSE.
 *
 */
void
mnb_home_grid_child_set_x_align (MnbHomeGrid  *grid,
                                 ClutterActor *child,
                                 MxAlign       align)
{
  MnbHomeGridChild *meta;

  g_return_if_fail (MNB_IS_HOME_GRID (grid));
  g_return_if_fail (CLUTTER_IS_ACTOR (child));

  meta = get_child_meta (grid, child);

  switch (align)
    {
    case MX_ALIGN_START:
      meta->x_align = 0.0;
      break;
    case MX_ALIGN_MIDDLE:
      meta->x_align = 0.5;
      break;
    case MX_ALIGN_END:
      meta->x_align = 1.0;
      break;
    }

  clutter_actor_queue_relayout (child);
}

/**
 * mnb_home_grid_child_get_y_align:
 * @grid: A #MnbHomeGrid
 * @child: A #ClutterActor
 *
 * Get the y-align value of the child
 *
 * Returns: An #MxAlign value
 */
MxAlign
mnb_home_grid_child_get_y_align (MnbHomeGrid  *grid,
                                 ClutterActor *child)
{
  MnbHomeGridChild *meta;

  g_return_val_if_fail (MNB_IS_HOME_GRID (grid), 0);
  g_return_val_if_fail (CLUTTER_IS_ACTOR (child), 0);

  meta = get_child_meta (grid, child);

  if (meta->y_align == 0.0)
    return MX_ALIGN_START;
  else if (meta->y_align == 1.0)
    return MX_ALIGN_END;
  else
    return MX_ALIGN_MIDDLE;

}

/**
 * mnb_home_grid_child_set_y_align:
 * @grid: A #MnbHomeGrid
 * @child: A #ClutterActor
 * @align: A #MxAlign value
 *
 * Set the value of the y-align property. This will only have an effect if
 * y-fill value is set to FALSE.
 *
 */
void
mnb_home_grid_child_set_y_align (MnbHomeGrid  *grid,
                                 ClutterActor *child,
                                 MxAlign       align)
{
  MnbHomeGridChild *meta;

  g_return_if_fail (MNB_IS_HOME_GRID (grid));
  g_return_if_fail (CLUTTER_IS_ACTOR (child));

  meta = get_child_meta (grid, child);

  switch (align)
    {
    case MX_ALIGN_START:
      meta->y_align = 0.0;
      break;
    case MX_ALIGN_MIDDLE:
      meta->y_align = 0.5;
      break;
    case MX_ALIGN_END:
      meta->y_align = 1.0;
      break;
    }

  clutter_actor_queue_relayout (child);
}
