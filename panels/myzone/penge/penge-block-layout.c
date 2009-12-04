/*
 *
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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

#include "penge-block-layout.h"

/* Child class */

typedef struct _PengeBlockLayoutChild PengeBlockLayoutChild;
typedef struct _ClutterLayoutMetaClass PengeBlockLayoutChildClass;

struct _PengeBlockLayoutChild
{
  ClutterLayoutMeta parent_instance;

  gint col_span;
};

enum
{
  PROP_CHILD_0,

  PROP_CHILD_COL_SPAN
};

#define PENGE_TYPE_BLOCK_LAYOUT_CHILD (penge_block_layout_child_get_type())

G_DEFINE_TYPE (PengeBlockLayoutChild,
               penge_block_layout_child,
               CLUTTER_TYPE_LAYOUT_META);

static gint
penge_block_layout_get_child_column_span (ClutterLayoutManager *manager,
                                          ClutterContainer     *container,
                                          ClutterActor         *child)
{
  PengeBlockLayoutChild *child_meta;

  child_meta = (PengeBlockLayoutChild *)
    clutter_layout_manager_get_child_meta (manager,
                                           container,
                                           child);

  return child_meta->col_span;
}

static void
penge_block_layout_child_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  PengeBlockLayoutChild *child = (PengeBlockLayoutChild *)object;

  switch (prop_id)
  {
    case PROP_CHILD_COL_SPAN:
      child->col_span = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
penge_block_layout_child_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  PengeBlockLayoutChild *child = (PengeBlockLayoutChild *)object;

  switch (prop_id)
  {
    case PROP_CHILD_COL_SPAN:
      g_value_set_int (value, child->col_span);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
penge_block_layout_child_class_init (PengeBlockLayoutChildClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  object_class->set_property = penge_block_layout_child_set_property;
  object_class->get_property = penge_block_layout_child_get_property;

  pspec = g_param_spec_int ("col-span",
                            "Column span",
                            "Number of columns for this actor to span",
                            1,
                            G_MAXINT,
                            1,
                            G_PARAM_READWRITE);
  g_object_class_install_property (object_class,
                                   PROP_CHILD_COL_SPAN,
                                   pspec);
}

static void
penge_block_layout_child_init (PengeBlockLayoutChild *self)
{
  self->col_span = 1;
}

/* Main class */

G_DEFINE_TYPE (PengeBlockLayout, penge_block_layout, CLUTTER_TYPE_LAYOUT_MANAGER)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_BLOCK_LAYOUT, PengeBlockLayoutPrivate))

typedef struct _PengeBlockLayoutPrivate PengeBlockLayoutPrivate;

struct _PengeBlockLayoutPrivate {
  gfloat prev_height;
  gfloat prev_width;

  gint column_count;
  gint row_count;

  gint padding;
  gfloat spacing;

  gfloat min_tile_height;
  gfloat min_tile_width;

  gfloat actual_tile_height;
  gfloat actual_tile_width;
};

enum
{
  COUNT_CHANGED_SIGNAL,
  LAST_SIGNAL
};

guint signals[LAST_SIGNAL] = { 0, };

enum
{
  PROP_0,
  PROP_SPACING,
  PROP_MIN_TILE_HEIGHT,
  PROP_MIN_TILE_WIDTH
};

static void
penge_block_layout_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeBlockLayoutPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_SPACING:
      g_value_set_int (value, priv->spacing);
      break;
    case PROP_MIN_TILE_HEIGHT:
      g_value_set_int (value, priv->min_tile_height);
      break;
    case PROP_MIN_TILE_WIDTH:
      g_value_set_int (value, priv->min_tile_width);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_block_layout_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeBlockLayoutPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_SPACING:
      priv->spacing = g_value_get_float (value);
      clutter_layout_manager_layout_changed (CLUTTER_LAYOUT_MANAGER (object));
      break;
    case PROP_MIN_TILE_HEIGHT:
      priv->min_tile_height = g_value_get_float (value);
      clutter_layout_manager_layout_changed (CLUTTER_LAYOUT_MANAGER (object));
      break;
    case PROP_MIN_TILE_WIDTH:
      priv->min_tile_width = g_value_get_float (value);
      clutter_layout_manager_layout_changed (CLUTTER_LAYOUT_MANAGER (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_block_layout_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_block_layout_parent_class)->dispose (object);
}

static void
penge_block_layout_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_block_layout_parent_class)->finalize (object);
}

static void
penge_block_layout_get_preferred_width (ClutterLayoutManager *manager,
                                        ClutterContainer     *container,
                                        gfloat                for_height,
                                        gfloat               *min_width_p,
                                        gfloat               *nat_width_p)
{
  PengeBlockLayoutPrivate *priv = GET_PRIVATE (manager);

  if (min_width_p)
    *min_width_p = priv->min_tile_width;

  if (nat_width_p)
    *nat_width_p = priv->min_tile_width;
}

static void
penge_block_layout_get_preferred_height (ClutterLayoutManager *manager,
                                         ClutterContainer     *container,
                                         gfloat                for_width,
                                         gfloat               *min_height_p,
                                         gfloat               *nat_height_p)
{
  PengeBlockLayoutPrivate *priv = GET_PRIVATE (manager);

  if (min_height_p)
    *min_height_p = priv->min_tile_height;

  if (nat_height_p)
    *nat_height_p = priv->min_tile_height;

}

static gint
_calculate_potential_column_count (PengeBlockLayout *pbl,
                                   gfloat            width)
{
  PengeBlockLayoutPrivate *priv = GET_PRIVATE (pbl);
  gfloat n;
  gint excess_width;

  n = (int)((width - 2 * priv->padding + priv->spacing) /
      ((int)priv->min_tile_width + (int)priv->spacing));

  excess_width = width -
    (n * priv->min_tile_width) -
    ((n - 1) * priv->spacing) -
    2 * priv->padding;
  priv->actual_tile_width = (int) (priv->min_tile_width + excess_width / n);

  return (gint)n;
}

static gint
_calculate_potential_row_count (PengeBlockLayout *pbl,
                                gfloat            height)
{
  PengeBlockLayoutPrivate *priv = GET_PRIVATE (pbl);
  gfloat n;
  gfloat ratio;

  ratio = priv->actual_tile_width / priv->min_tile_width;

  priv->actual_tile_height = (int) (priv->min_tile_height * ratio);

  n = (int)((height - 2 * priv->padding + priv->spacing) /
      ((int)priv->actual_tile_height + (int)priv->spacing));

  return (gint)n;
}

static void
penge_block_layout_generate_counts (PengeBlockLayout *layout,
                                    gfloat            width,
                                    gfloat            height)
{
  PengeBlockLayoutPrivate *priv = GET_PRIVATE (layout);
  gint row_count, column_count;

  /* The width and height has changed. We need to recalculate our counts */

  /* Must do it in this order since row count depends on column count */
  column_count = _calculate_potential_column_count (layout, width);
  row_count = _calculate_potential_row_count (layout, height);

  if (priv->column_count != column_count || priv->row_count != row_count)
  {
    priv->column_count = column_count;
    priv->row_count = row_count;

    if (column_count * row_count > 0)
    {
      g_signal_emit (layout,
                     signals[COUNT_CHANGED_SIGNAL],
                     0,
                     column_count * row_count);

      g_debug (G_STRLOC ": Possible row count = %d, possible column count = %d"
               " for width = %f and height = %f and child width = %f and height = %f",
               priv->row_count,
               priv->column_count,
               width,
               height,
               priv->actual_tile_width,
               priv->actual_tile_height);
    }
  }
}

static void
penge_block_layout_allocate (ClutterLayoutManager   *layout,
                             ClutterContainer       *container,
                             const ClutterActorBox  *box,
                             ClutterAllocationFlags  flags)
{
  PengeBlockLayoutPrivate *priv = GET_PRIVATE (layout);
  GList *l, *children;
  gfloat width, height;
  gint col_span;
  gint y_count, x_count;
  ClutterActor *child;
  ClutterActorBox child_box;

  width = box->x2 - box->x1;
  height = box->y2 - box->y1;

  if (priv->prev_width != width || priv->prev_height != height)
  {
    penge_block_layout_generate_counts ((PengeBlockLayout *)layout,
                                        width,
                                        height);
  }

  children = clutter_container_get_children (container);

  if (!children)
    return;

  children = g_list_reverse (children);

  for (y_count = 0; y_count < priv->row_count; y_count++)
  {
    for (x_count = 0; x_count < priv->column_count; /* nop */)
    {
      /* Find first that can fit */
      for (l = children; l; l = l->next)
      {
        child = (ClutterActor *)l->data;

        col_span = penge_block_layout_get_child_column_span (layout,
                                                             container,
                                                             child);

        /* Check if it fits */
        if (x_count + col_span <= priv->column_count)
        {
          child_box.x1 = x_count * priv->actual_tile_width +
                         x_count * priv->spacing +
                         priv->padding;
          child_box.x2 = child_box.x1 +
                         priv->actual_tile_width * col_span +
                         priv->spacing * (col_span - 1);
          child_box.y1 = y_count * priv->actual_tile_height +
                         y_count * priv->spacing +
                         priv->padding;
          child_box.y2 = child_box.y1 + priv->actual_tile_height;

          if (child_box.x2 > width || child_box.y2 > height)
          {
            break;
          }

          clutter_actor_allocate (child, &child_box, flags);
          clutter_actor_show (child);

          children = g_list_remove (children, child);

          x_count += col_span;
          break;
        }
      }

      if (!children)
        break;

    }

    if (!children)
      break;
  }

  g_list_free (children);
}

static GType
penge_block_layout_get_child_meta (ClutterLayoutManager *manager)
{
  return PENGE_TYPE_BLOCK_LAYOUT_CHILD;
}

static void
penge_block_layout_class_init (PengeBlockLayoutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterLayoutManagerClass *layout_class = CLUTTER_LAYOUT_MANAGER_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeBlockLayoutPrivate));

  object_class->get_property = penge_block_layout_get_property;
  object_class->set_property = penge_block_layout_set_property;
  object_class->dispose = penge_block_layout_dispose;
  object_class->finalize = penge_block_layout_finalize;

  layout_class->get_preferred_width = penge_block_layout_get_preferred_width;
  layout_class->get_preferred_height = penge_block_layout_get_preferred_height;
  layout_class->allocate = penge_block_layout_allocate;
  layout_class->get_child_meta_type = penge_block_layout_get_child_meta;

  pspec = g_param_spec_float ("min-tile-width",
                              "Minimum child width",
                              "The minimum width to use for the child",
                              0.0,
                              G_MAXFLOAT,
                              0.0,
                              G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_MIN_TILE_WIDTH, pspec);

  pspec = g_param_spec_float ("min-tile-height",
                              "Minimum child height",
                              "The minimum width to use for the child",
                              0.0,
                              G_MAXFLOAT,
                              0.0,
                              G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_MIN_TILE_HEIGHT, pspec);

  pspec = g_param_spec_float ("spacing",
                              "Spacing",
                              "The spacing to use between rows and columns",
                              0.0,
                              G_MAXFLOAT,
                              0.0,
                              G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SPACING, pspec);

  signals[COUNT_CHANGED_SIGNAL] =
    g_signal_new ("count-changed",
                  PENGE_TYPE_BLOCK_LAYOUT,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_INT);
}

static void
penge_block_layout_init (PengeBlockLayout *self)
{
}

ClutterLayoutManager *
penge_block_layout_new (void)
{
  return g_object_new (PENGE_TYPE_BLOCK_LAYOUT, NULL);
}

void
penge_block_layout_set_min_tile_size (PengeBlockLayout *pbl,
                                      gfloat            width,
                                      gfloat            height)
{
  PengeBlockLayoutPrivate *priv = GET_PRIVATE (pbl);

  priv->min_tile_width = width;
  priv->min_tile_height = height;

  g_object_notify (G_OBJECT (pbl), "min-tile-width");
  g_object_notify (G_OBJECT (pbl), "min-tile-height");

  clutter_layout_manager_layout_changed (CLUTTER_LAYOUT_MANAGER (pbl));
}

void
penge_block_layout_set_spacing (PengeBlockLayout *pbl,
                                gfloat            spacing)
{
  PengeBlockLayoutPrivate *priv = GET_PRIVATE (pbl);

  priv->spacing = spacing;

  g_object_notify (G_OBJECT (pbl), "spacing");

  clutter_layout_manager_layout_changed (CLUTTER_LAYOUT_MANAGER (pbl));
}
