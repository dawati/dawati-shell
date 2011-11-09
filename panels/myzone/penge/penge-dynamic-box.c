/*
 * Copyright (C) 2010, Intel Corporation.
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

#include "penge-dynamic-box.h"

static void penge_dynamic_box_iface_init (ClutterContainerIface *iface);

G_DEFINE_TYPE_WITH_CODE (PengeDynamicBox, penge_dynamic_box, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                penge_dynamic_box_iface_init))

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_DYNAMIC_BOX, PengeDynamicBoxPrivate))
#define GET_PRIVATE(o) ((PengeDynamicBox *)o)->priv

struct _PengeDynamicBoxPrivate {
  GList *children;
};

static void
penge_dynamic_box_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_dynamic_box_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_dynamic_box_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_dynamic_box_parent_class)->dispose (object);
}

static void
penge_dynamic_box_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_dynamic_box_parent_class)->finalize (object);
}

#define SPACING 4.0
static void
penge_dynamic_box_allocate (ClutterActor          *actor,
                            const ClutterActorBox *box,
                            ClutterAllocationFlags flags)
{
  PengeDynamicBoxPrivate *priv = GET_PRIVATE (actor);
  gfloat width, height;
  MxPadding padding = { 0, };
  ClutterActorBox child_box;
  ClutterActorBox zero_box = { 0, };
  GList *l;
  gfloat last_y;

  CLUTTER_ACTOR_CLASS (penge_dynamic_box_parent_class)->allocate (actor,
                                                                  box,
                                                                  flags);

  width = box->x2 - box->x1;
  height = box->y2 - box->y1;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  last_y = padding.top;

  for (l = priv->children; l; l = l->next)
  {
    ClutterActor *child = (ClutterActor *)l->data;
    gfloat child_nat_h;

    clutter_actor_get_preferred_height (child,
                                        width - (padding.left + padding.right),
                                        NULL,
                                        &child_nat_h);
    child_box.y1 = last_y;
    child_box.x1 = padding.left;
    child_box.x2 = width - padding.right;
    child_box.y2 = child_nat_h + last_y;
    last_y = child_box.y2;

    if (last_y <= height - padding.bottom)
      clutter_actor_allocate (child, &child_box, flags);
    else
      clutter_actor_allocate (child, &zero_box, flags);

    last_y += SPACING;
  }
}


static void
penge_dynamic_box_paint (ClutterActor *actor)
{
  PengeDynamicBoxPrivate *priv = GET_PRIVATE (actor);
  GList *l;

  for (l = priv->children; l; l = l->next)
  {
    ClutterActor *child = (ClutterActor *)l->data;
    ClutterActorBox box;

    clutter_actor_get_allocation_box (child,
                                      &box);

    if (box.x2 - box.x1 > 0 && box.y2 - box.y1 > 0)
    {
      clutter_actor_paint (child);
    }
  }
}


static void
penge_dynamic_box_pick (ClutterActor       *actor,
                        const ClutterColor *color)
{
  PengeDynamicBoxPrivate *priv = GET_PRIVATE (actor);
  GList *l;

  for (l = priv->children; l; l = l->next)
  {
    ClutterActor *child = (ClutterActor *)l->data;
    ClutterActorBox box;

    clutter_actor_get_allocation_box (child,
                                      &box);

    if (box.x2 - box.x1 > 0 && box.y2 - box.y1 > 0)
    {
      clutter_actor_paint (child);
    }
  }
}

static gfloat
_calculate_children_height (PengeDynamicBox *box,
                            gfloat           for_width)
{
  PengeDynamicBoxPrivate *priv = GET_PRIVATE (box);
  GList *l;
  MxPadding padding = { 0, };
  gfloat height = 0, child_nat_h;

  mx_widget_get_padding (MX_WIDGET (box), &padding);
  height += padding.top + padding.bottom;

  /* Subtract padding */
  for_width -= (padding.left + padding.right);

  for (l = priv->children; l; l = l->next)
  {
    ClutterActor *child = (ClutterActor *)l->data;

    clutter_actor_get_preferred_height (child,
                                        for_width,
                                        NULL,
                                        &child_nat_h);

    height += child_nat_h;

    /* No SPACING on last */
    if (l->next != NULL)
      height += SPACING;
  }

  return height;
}

static void
penge_dynamic_box_get_preferred_height (ClutterActor *actor,
                                        gfloat        for_width,
                                        gfloat       *min_height_p,
                                        gfloat       *nat_height_p)
{
  PengeDynamicBox *box = (PengeDynamicBox *)actor;

  if (min_height_p)
    *min_height_p = 0;

  /* Report our natural height to be our potential maximum */
  if (nat_height_p)
  {
    *nat_height_p = _calculate_children_height (box,
                                                for_width);
  }
}

static void
penge_dynamic_box_class_init (PengeDynamicBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeDynamicBoxPrivate));

  object_class->get_property = penge_dynamic_box_get_property;
  object_class->set_property = penge_dynamic_box_set_property;
  object_class->dispose = penge_dynamic_box_dispose;
  object_class->finalize = penge_dynamic_box_finalize;

  actor_class->allocate = penge_dynamic_box_allocate;
  actor_class->paint = penge_dynamic_box_paint;
  actor_class->pick = penge_dynamic_box_pick;
  actor_class->get_preferred_height = penge_dynamic_box_get_preferred_height;
}

static void
penge_dynamic_box_init (PengeDynamicBox *self)
{
  PengeDynamicBoxPrivate *priv = GET_PRIVATE_REAL (self);

  self->priv = priv;
}

ClutterActor *
penge_dynamic_box_new (void)
{
  return g_object_new (PENGE_TYPE_DYNAMIC_BOX, NULL);
}

static void
penge_dynamic_box_add_actor (ClutterContainer *container,
                             ClutterActor     *actor)
{
  PengeDynamicBoxPrivate *priv = GET_PRIVATE (container);

  clutter_actor_set_parent (actor, CLUTTER_ACTOR (container));

  priv->children = g_list_append (priv->children, actor);

  g_signal_emit_by_name (container, "actor-added", actor);
}

static void
penge_dynamic_box_remove_actor (ClutterContainer *container,
                                ClutterActor     *actor)
{
  PengeDynamicBoxPrivate *priv = GET_PRIVATE (container);
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
penge_dynamic_box_foreach (ClutterContainer *container,
                           ClutterCallback   callback,
                           gpointer          callback_data)
{
  PengeDynamicBoxPrivate *priv = GET_PRIVATE (container);

  g_list_foreach (priv->children, (GFunc) callback, callback_data);
}

static void
penge_dynamic_box_raise (ClutterContainer *container,
                         ClutterActor     *actor,
                         ClutterActor     *sibling)
{
  PengeDynamicBoxPrivate *priv = GET_PRIVATE (container);

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
penge_dynamic_box_lower (ClutterContainer *container,
                         ClutterActor     *actor,
                         ClutterActor     *sibling)
{
  PengeDynamicBoxPrivate *priv = GET_PRIVATE (container);

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
penge_dynamic_box_sort_depth_order (ClutterContainer *container)
{
  PengeDynamicBoxPrivate *priv = GET_PRIVATE (container);

  priv->children = g_list_sort (priv->children, sort_by_depth);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (container));
}

static void
penge_dynamic_box_iface_init (ClutterContainerIface *iface)
{
  iface->add = penge_dynamic_box_add_actor;
  iface->remove = penge_dynamic_box_remove_actor;

  iface->foreach = penge_dynamic_box_foreach;

  iface->lower = penge_dynamic_box_lower;
  iface->raise = penge_dynamic_box_raise;
  iface->sort_depth_order = penge_dynamic_box_sort_depth_order;
}

