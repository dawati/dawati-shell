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

#include "penge-block-container.h"

/* Child class */

typedef struct _PengeBlockContainerChild PengeBlockContainerChild;
typedef struct _ClutterChildMetaClass PengeBlockContainerChildClass;

struct _PengeBlockContainerChild
{
  ClutterChildMeta parent_instance;

  gint col_span;
};

enum
{
  PROP_CHILD_0,

  PROP_CHILD_COL_SPAN
};

#define PENGE_TYPE_BLOCK_CONTAINER_CHILD (penge_block_container_child_get_type())

G_DEFINE_TYPE (PengeBlockContainerChild,
               penge_block_container_child,
               CLUTTER_TYPE_CHILD_META);

static gint
penge_block_container_get_child_column_span (PengeBlockContainer *container,
                                             ClutterActor        *child)
{
  PengeBlockContainerChild *child_meta;

  child_meta = (PengeBlockContainerChild *)
    clutter_container_get_child_meta ((ClutterContainer *)container,
                                      child);

  return child_meta->col_span;
}

static void
penge_block_container_child_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  PengeBlockContainerChild *child = (PengeBlockContainerChild *)object;

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
penge_block_container_child_get_property (GObject    *object,
                                          guint       prop_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  PengeBlockContainerChild *child = (PengeBlockContainerChild *)object;

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
penge_block_container_child_class_init (PengeBlockContainerChildClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  object_class->set_property = penge_block_container_child_set_property;
  object_class->get_property = penge_block_container_child_get_property;

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
penge_block_container_child_init (PengeBlockContainerChild *self)
{
  self->col_span = 1;
}

/* Main class */

static void penge_block_container_iface_init (ClutterContainerIface *iface);

G_DEFINE_TYPE_WITH_CODE (PengeBlockContainer, penge_block_container, CLUTTER_TYPE_ACTOR,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                penge_block_container_iface_init))

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_BLOCK_CONTAINER, PengeBlockContainerPrivate))


#define GET_PRIVATE(o) ((PengeBlockContainer *)o)->priv

struct _PengeBlockContainerPrivate {
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

  GList *children;
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
penge_block_container_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (object);

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
penge_block_container_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_SPACING:
      priv->spacing = g_value_get_float (value);
      clutter_actor_queue_relayout (CLUTTER_ACTOR (object));
      break;
    case PROP_MIN_TILE_HEIGHT:
      priv->min_tile_height = g_value_get_float (value);
      clutter_actor_queue_relayout (CLUTTER_ACTOR (object));
      break;
    case PROP_MIN_TILE_WIDTH:
      priv->min_tile_width = g_value_get_float (value);
      clutter_actor_queue_relayout (CLUTTER_ACTOR (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_block_container_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_block_container_parent_class)->dispose (object);
}

static void
penge_block_container_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_block_container_parent_class)->finalize (object);
}

static void
penge_block_container_get_preferred_width (ClutterActor *actor,
                                           gfloat        for_height,
                                           gfloat       *min_width_p,
                                           gfloat       *nat_width_p)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (actor);

  if (min_width_p)
    *min_width_p = priv->min_tile_width;

  if (nat_width_p)
    *nat_width_p = priv->min_tile_width;
}

static void
penge_block_container_get_preferred_height (ClutterActor *actor,
                                            gfloat        for_width,
                                            gfloat       *min_height_p,
                                            gfloat       *nat_height_p)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (actor);

  if (min_height_p)
    *min_height_p = priv->min_tile_height;

  if (nat_height_p)
    *nat_height_p = priv->min_tile_height;

}

static gint
_calculate_potential_column_count (PengeBlockContainer *pbc,
                                   gfloat               width)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (pbc);
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
_calculate_potential_row_count (PengeBlockContainer *pbc,
                                gfloat               height)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (pbc);
  gfloat n;
  gfloat ratio;

  ratio = priv->actual_tile_width / priv->min_tile_width;

  priv->actual_tile_height = (int) (priv->min_tile_height * ratio);

  n = (int)((height - 2 * priv->padding + priv->spacing) /
      ((int)priv->actual_tile_height + (int)priv->spacing));

  return (gint)n;
}

static void
penge_block_container_generate_counts (PengeBlockContainer *pbc,
                                       gfloat               width,
                                       gfloat               height)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (pbc);
  gint row_count, column_count;

  /* The width and height has changed. We need to recalculate our counts */

  /* Must do it in this order since row count depends on column count */
  column_count = _calculate_potential_column_count (pbc, width);
  row_count = _calculate_potential_row_count (pbc, height);

  if (priv->column_count != column_count || priv->row_count != row_count)
  {
    priv->column_count = column_count;
    priv->row_count = row_count;

    if (column_count * row_count > 0)
    {
      g_signal_emit (pbc,
                     signals[COUNT_CHANGED_SIGNAL],
                     0,
                     column_count * row_count);
    }
  }
}

static void
penge_block_container_allocate (ClutterActor           *actor,
                                const ClutterActorBox  *box,
                                ClutterAllocationFlags  flags)
{
  PengeBlockContainer *pbc = PENGE_BLOCK_CONTAINER (actor);
  PengeBlockContainerPrivate *priv = GET_PRIVATE (actor);
  GList *l, *children;
  gfloat width, height;
  gint col_span;
  gint y_count, x_count;
  ClutterActor *child;
  ClutterActorBox child_box;
  gboolean fit_found = FALSE;

  width = box->x2 - box->x1;
  height = box->y2 - box->y1;

  CLUTTER_ACTOR_CLASS (penge_block_container_parent_class)->allocate (actor,
                                                                      box,
                                                                      flags);


  if (priv->prev_width != width || priv->prev_height != height)
  {
    penge_block_container_generate_counts (pbc,
                                           width,
                                           height);
  }

  children = g_list_copy (priv->children);

  if (!children)
    return;

  children = g_list_reverse (children);

  for (y_count = 0; y_count < priv->row_count; y_count++)
  {
    for (x_count = 0; x_count < priv->column_count; /* nop */)
    {
      /* Find first that can fit */
      fit_found = FALSE;

      for (l = children; l; l = l->next)
      {
        child = (ClutterActor *)l->data;
        col_span = penge_block_container_get_child_column_span (pbc,
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

          clutter_actor_allocate (child, &child_box, flags);
          children = g_list_remove (children, child);

          /* Increment with the col-span to take into consideration how many
           * "blocks" we are occupying
           */
          x_count += col_span;

          fit_found = TRUE;
          break;
        }
      }

      /* If we didn't find something that fits then we just leave a gap. This
       * is basically fine.
       */
      if (!fit_found)
      {
        /* We couldn't find something that fitted */
        break;
      }
    }

    if (!children)
      break;
  }

  /* Allocate all children we don't get round to with {0,0,0,0}. We skip such
   * children in the paint.
   */
  for (l = children; l; l = l->next)
  {
    ClutterActor *actor = CLUTTER_ACTOR (l->data);
    ClutterActorBox zero_box = { 0, };
    clutter_actor_allocate (actor, &zero_box, flags);
  }

  g_list_free (children);
}

/* These are needed since the layout manager allocates {0, 0, 0, 0} for
 * children it can't lay out.
 */
static void
_paint_foreach_cb (ClutterActor *actor,
                   gpointer      data)
{
  ClutterActorBox actor_box;
  gfloat w, h;

  clutter_actor_get_allocation_box (actor, &actor_box);

  clutter_actor_box_get_size (&actor_box, &w, &h);

  if (w > 0 && h > 0)
  {
    clutter_actor_paint (actor);
  }
}

static void
penge_block_container_pick (ClutterActor       *actor,
                            const ClutterColor *color)
{
  clutter_container_foreach (CLUTTER_CONTAINER (actor),
                             (ClutterCallback)_paint_foreach_cb,
                             actor);
}

static void
penge_block_container_paint (ClutterActor *actor)
{
  clutter_container_foreach (CLUTTER_CONTAINER (actor),
                             (ClutterCallback)_paint_foreach_cb,
                             actor);
}

static void
penge_block_container_class_init (PengeBlockContainerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeBlockContainerPrivate));

  object_class->get_property = penge_block_container_get_property;
  object_class->set_property = penge_block_container_set_property;
  object_class->dispose = penge_block_container_dispose;
  object_class->finalize = penge_block_container_finalize;

  actor_class->get_preferred_width = penge_block_container_get_preferred_width;
  actor_class->get_preferred_height = penge_block_container_get_preferred_height;
  actor_class->allocate = penge_block_container_allocate;
  actor_class->paint = penge_block_container_paint;
  actor_class->pick = penge_block_container_pick;

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
                  PENGE_TYPE_BLOCK_CONTAINER,
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
penge_block_container_add_actor (ClutterContainer *container,
                                 ClutterActor     *actor)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (container);

  clutter_actor_set_parent (actor, CLUTTER_ACTOR (container));

  priv->children = g_list_append (priv->children, actor);

  g_signal_emit_by_name (container, "actor-added", actor);
}

static void
penge_block_container_remove_actor (ClutterContainer *container,
                                    ClutterActor     *actor)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (container);
  GList *item = NULL;

  item = g_list_find (priv->children, actor);

  if (item == NULL)
  {
    g_warning ("Actor of type '%s' is not a child of container of type '%s'",
               g_type_name (G_OBJECT_TYPE (actor)),
               g_type_name (G_OBJECT_TYPE (container)));
    return;
  }

  g_object_ref (actor);

  priv->children = g_list_delete_link (priv->children, item);
  clutter_actor_unparent (actor);

  g_signal_emit_by_name (container, "actor-removed", actor);

  g_object_unref (actor);
}

static void
penge_block_container_foreach (ClutterContainer *container,
                               ClutterCallback   callback,
                               gpointer          callback_data)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (container);

  g_list_foreach (priv->children, (GFunc) callback, callback_data);
}

static void
penge_block_container_raise (ClutterContainer *container,
                             ClutterActor     *actor,
                             ClutterActor     *sibling)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (container);

  priv->children = g_list_remove (priv->children, actor);

  if (sibling == NULL)
    priv->children = g_list_append (priv->children, actor);
  else
    {
      gint index_ = g_list_index (priv->children, sibling) + 1;

      priv->children = g_list_insert (priv->children, actor, index_);
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (container));
}

static void
penge_block_container_lower (ClutterContainer *container,
                             ClutterActor     *actor,
                             ClutterActor     *sibling)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (container);

  priv->children = g_list_remove (priv->children, actor);

  if (sibling == NULL)
    priv->children = g_list_prepend (priv->children, actor);
  else
    {
      gint index_ = g_list_index (priv->children, sibling);

      priv->children = g_list_insert (priv->children, actor, index_);
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (container));
}

static gint
sort_by_depth (gconstpointer a,
               gconstpointer b)
{
  gfloat depth_a = clutter_actor_get_depth ((ClutterActor *) a);
  gfloat depth_b = clutter_actor_get_depth ((ClutterActor *) b);

  if (depth_a < depth_b)
    return -1;

  if (depth_a > depth_b)
    return 1;

  return 0;
}

static void
penge_block_container_sort_depth_order (ClutterContainer *container)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (container);

  priv->children = g_list_sort (priv->children, sort_by_depth);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (container));
}

static void
penge_block_container_init (PengeBlockContainer *self)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE_REAL (self);
  self->priv = priv;
}

static void
penge_block_container_iface_init (ClutterContainerIface *iface)
{
  iface->add = penge_block_container_add_actor;
  iface->remove = penge_block_container_remove_actor;

  iface->foreach = penge_block_container_foreach;

  iface->lower = penge_block_container_lower;
  iface->raise = penge_block_container_raise;
  iface->sort_depth_order = penge_block_container_sort_depth_order;

  iface->child_meta_type = PENGE_TYPE_BLOCK_CONTAINER_CHILD;
}

ClutterActor *
penge_block_container_new (void)
{
  return g_object_new (PENGE_TYPE_BLOCK_CONTAINER, NULL);
}

void
penge_block_container_set_min_tile_size (PengeBlockContainer *pbc,
                                         gfloat               width,
                                         gfloat               height)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (pbc);

  priv->min_tile_width = width;
  priv->min_tile_height = height;

  g_object_notify (G_OBJECT (pbc), "min-tile-width");
  g_object_notify (G_OBJECT (pbc), "min-tile-height");

  clutter_actor_queue_relayout (CLUTTER_ACTOR (pbc));
}

void
penge_block_container_set_spacing (PengeBlockContainer *pbc,
                                   gfloat               spacing)
{
  PengeBlockContainerPrivate *priv = GET_PRIVATE (pbc);

  priv->spacing = spacing;

  g_object_notify (G_OBJECT (pbc), "spacing");

  clutter_actor_queue_relayout (CLUTTER_ACTOR (pbc));
}

