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
 *
 */

#include "penge-magic-container.h"

static void _container_iface_init (ClutterContainerIface *iface);

G_DEFINE_TYPE_WITH_CODE (PengeMagicContainer,
                         penge_magic_container,
                         CLUTTER_TYPE_ACTOR,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                _container_iface_init));

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_MAGIC_CONTAINER, PengeMagicContainerPrivate))

typedef struct _PengeMagicContainerPrivate PengeMagicContainerPrivate;

struct _PengeMagicContainerPrivate {
  GList *children;

  gfloat prev_height;
  gfloat prev_width;

  gint column_count;
  gint row_count;

  gint padding;
  gint spacing;

  gfloat min_tile_height;
  gfloat min_tile_width;

  gfloat actual_tile_height;
  gfloat actual_tile_width;
};

enum
{
  PROP_0,
  PROP_CHILD_WIDTH,
  PROP_CHILD_HEIGHT
};

enum
{
  COUNT_CHANGED_SIGNAL,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
penge_magic_container_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeMagicContainer *pmc = (PengeMagicContainer *)object;
  PengeMagicContainerPrivate *priv = GET_PRIVATE (pmc);

  switch (property_id) {
    case PROP_CHILD_WIDTH:
      g_value_set_float (value, priv->min_tile_width);
      break;
    case PROP_CHILD_HEIGHT:
      g_value_set_float (value, priv->min_tile_height);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_magic_container_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeMagicContainer *pmc = (PengeMagicContainer *)object;
  PengeMagicContainerPrivate *priv = GET_PRIVATE (pmc);

  switch (property_id) {
    case PROP_CHILD_WIDTH:
      penge_magic_container_set_minimum_child_size (pmc,
                                                    g_value_get_float (value),
                                                    priv->min_tile_height);
      break;
    case PROP_CHILD_HEIGHT:
      penge_magic_container_set_minimum_child_size (pmc,
                                                    priv->min_tile_width,
                                                    g_value_get_float (value));

      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_magic_container_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_magic_container_parent_class)->dispose (object);
}

static void
penge_magic_container_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_magic_container_parent_class)->finalize (object);
}

static gint
_calculate_potential_row_count (PengeMagicContainer *pmc,
                                gfloat               height)
{
  PengeMagicContainerPrivate *priv = GET_PRIVATE (pmc);
  gfloat n;
  gint excess_height;

  n = (int)((height - 2 * priv->padding + priv->spacing) /
      ((int)priv->min_tile_height + (int)priv->spacing));

  excess_height = height -
    (n * priv->min_tile_height) -
    ((n - 1) * priv->spacing) -
    2 * priv->padding;
  priv->actual_tile_height = (int) (priv->min_tile_height + excess_height / n);

  return (gint)n;
}

static gint
_calculate_potential_column_count (PengeMagicContainer *pmc,
                                   gfloat               width)
{
  PengeMagicContainerPrivate *priv = GET_PRIVATE (pmc);
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

static void
penge_magic_container_calculate_counts (PengeMagicContainer *pmc,
                                        gfloat               width,
                                        gfloat               height)
{
  PengeMagicContainerPrivate *priv = GET_PRIVATE (pmc);
  gint row_count, column_count;

  /* The width and height has changed. We need to recalculate our counts */
  row_count = _calculate_potential_row_count (pmc, height);
  column_count = _calculate_potential_column_count (pmc, width);

  if (priv->column_count != column_count || priv->row_count != row_count)
  {
    priv->column_count = column_count;
    priv->row_count = row_count;

    if (column_count * row_count > 0)
    {
      g_signal_emit (pmc, signals[COUNT_CHANGED_SIGNAL], 0, column_count * row_count);

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
penge_magic_container_allocate (ClutterActor          *actor,
                                const ClutterActorBox *box,
                                ClutterAllocationFlags flags)
{
  PengeMagicContainer *pmc = (PengeMagicContainer *)actor;
  PengeMagicContainerPrivate *priv = GET_PRIVATE (pmc);
  gfloat width, height;
  ClutterActorBox child_box;
  GList *l;
  gint x_count = 0, y_count = 0;
  ClutterActor *child;

  width = box->x2 - box->x1;
  height = box->y2 - box->y1;

  g_debug ("%f %f", width, height);

  if (CLUTTER_ACTOR_CLASS (penge_magic_container_parent_class)->allocate)
    CLUTTER_ACTOR_CLASS (penge_magic_container_parent_class)->allocate (actor, box, flags);

  if (priv->prev_width != width || priv->prev_height != height)
  {
    penge_magic_container_calculate_counts (pmc,
                                            width,
                                            height);
  }

  l = priv->children;

  for (y_count = 0; y_count < priv->row_count; y_count++)
  {
    for (x_count = 0; x_count < priv->column_count; x_count++)
    {
      if (!l)
        break;

      child = (ClutterActor *)l->data;

      child_box.x1 = x_count * priv->actual_tile_width +
                     x_count * priv->spacing +
                     priv->padding;
      child_box.x2 = child_box.x1 + priv->actual_tile_width;
      child_box.y1 = y_count * priv->actual_tile_height +
                     y_count * priv->spacing +
                     priv->padding;
      child_box.y2 = child_box.y1 + priv->actual_tile_height;

      clutter_actor_allocate (child, &child_box, flags);
      clutter_actor_show (child);
      l = l->next;
    }

    if (!l)
      break;
  }

  for (; l; l = l->next)
  {
    clutter_actor_hide ((ClutterActor *)l->data);
  }
}

static void
penge_magic_container_paint (ClutterActor *actor)
{
  PengeMagicContainer *pmc = (PengeMagicContainer *)actor;
  PengeMagicContainerPrivate *priv = GET_PRIVATE (pmc);
  GList *l;
  gint i = 0;
  ClutterActor *child;

  if (CLUTTER_ACTOR_CLASS (penge_magic_container_parent_class)->paint)
    CLUTTER_ACTOR_CLASS (penge_magic_container_parent_class)->paint (actor);

  for (i = 0, l = priv->children;
       i < priv->row_count * priv->column_count && l;
       i++, l = l->next)
  {
    child = (ClutterActor *)l->data;

    if (CLUTTER_ACTOR_IS_MAPPED (child))
      clutter_actor_paint (child);
  }
}

static void
penge_magic_container_pick (ClutterActor       *actor,
                            const ClutterColor *color)
{
  PengeMagicContainer *pmc = (PengeMagicContainer *)actor;
  PengeMagicContainerPrivate *priv = GET_PRIVATE (pmc);
  GList *l;
  gint i = 0;
  ClutterActor *child;

   /* Chain up so we get a bounding box painted (if we are reactive) */
  CLUTTER_ACTOR_CLASS (penge_magic_container_parent_class)->pick (actor, color);

  for (i = 0, l = priv->children;
       i < priv->row_count * priv->column_count && l;
       i++, l = l->next)
  {
    child = (ClutterActor *)l->data;

    if (CLUTTER_ACTOR_IS_MAPPED (child))
      clutter_actor_paint (child);
  }
}

static void
penge_magic_container_class_init (PengeMagicContainerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeMagicContainerPrivate));

  object_class->get_property = penge_magic_container_get_property;
  object_class->set_property = penge_magic_container_set_property;
  object_class->dispose = penge_magic_container_dispose;
  object_class->finalize = penge_magic_container_finalize;

  actor_class->allocate = penge_magic_container_allocate;
  actor_class->paint = penge_magic_container_paint;
  actor_class->pick = penge_magic_container_pick;

  pspec = g_param_spec_float ("min-child-width",
                              "Minimum child width",
                              "The minimum width to use for the child",
                              0.0,
                              G_MAXFLOAT,
                              0.0,
                              G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_CHILD_WIDTH, pspec);

  pspec = g_param_spec_float ("min-child-height",
                              "Minimum child height",
                              "The minimum width to use for the child",
                              0.0,
                              G_MAXFLOAT,
                              0.0,
                              G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_CHILD_HEIGHT, pspec);

  signals[COUNT_CHANGED_SIGNAL] =
    g_signal_new ("count-changed",
                  PENGE_TYPE_MAGIC_CONTAINER,
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
penge_magic_container_init (PengeMagicContainer *self)
{
  PengeMagicContainerPrivate *priv = GET_PRIVATE (self);

  priv->row_count = -1;
  priv->column_count = -1;

  priv->padding = 0;
  priv->spacing = 10;

  priv->min_tile_height = 100;
  priv->min_tile_width = 100;
}

ClutterActor *
penge_magic_container_new (void)
{
  return g_object_new (PENGE_TYPE_MAGIC_CONTAINER, NULL);
}

static void
penge_magic_container_add (ClutterContainer *container,
                           ClutterActor     *actor)
{
  PengeMagicContainer *pmc = (PengeMagicContainer *)container;
  PengeMagicContainerPrivate *priv = GET_PRIVATE (pmc);

  priv->children = g_list_append (priv->children, actor);
  clutter_actor_set_parent (actor, CLUTTER_ACTOR (container));
  g_signal_emit_by_name (container, "actor-added", actor);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (container));
}

static void
penge_magic_container_remove (ClutterContainer *container,
                              ClutterActor     *actor)
{
  PengeMagicContainer *pmc = (PengeMagicContainer *)container;
  PengeMagicContainerPrivate *priv = GET_PRIVATE (pmc);

  priv->children = g_list_remove (priv->children, actor);

  g_object_ref (actor);
  clutter_actor_unparent (actor);
  g_signal_emit_by_name (container, "actor-removed", actor);
  g_object_unref (actor);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (container));
}

static void
penge_magic_container_foreach (ClutterContainer *container,
                               ClutterCallback   callback,
                               gpointer          userdata)
{
  PengeMagicContainer *pmc = (PengeMagicContainer *)container;
  PengeMagicContainerPrivate *priv = GET_PRIVATE (pmc);
  g_list_foreach (priv->children,
                  (GFunc)callback,
                  userdata);
}

static void
_container_iface_init (ClutterContainerIface *iface)
{
  iface->add = penge_magic_container_add;
  iface->remove = penge_magic_container_remove;
  iface->foreach = penge_magic_container_foreach;
}

void
penge_magic_container_set_minimum_child_size (PengeMagicContainer *pmc,
                                              gfloat               width,
                                              gfloat               height)
{
  PengeMagicContainerPrivate *priv = GET_PRIVATE (pmc);

  priv->min_tile_width = width;
  priv->min_tile_height = height;

  penge_magic_container_calculate_counts (pmc,
                                          clutter_actor_get_width ((ClutterActor *)pmc),
                                          clutter_actor_get_height ((ClutterActor *)pmc));

  clutter_actor_queue_relayout ((ClutterActor *)pmc);
  g_object_notify ((GObject *)pmc, "min-child-width");
  g_object_notify ((GObject *)pmc, "min-child-height");
}

