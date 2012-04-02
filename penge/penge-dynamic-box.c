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

G_DEFINE_TYPE (PengeDynamicBox, penge_dynamic_box, MX_TYPE_WIDGET)

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
  gfloat width, height;
  MxPadding padding = { 0, };
  ClutterActorBox child_box;
  ClutterActorBox zero_box = { 0, };
  gfloat last_y;
  ClutterActorIter iter;
  ClutterActor *child;

  CLUTTER_ACTOR_CLASS (penge_dynamic_box_parent_class)->allocate (actor,
                                                                  box,
                                                                  flags);

  width = box->x2 - box->x1;
  height = box->y2 - box->y1;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  last_y = padding.top;

  clutter_actor_iter_init (&iter, actor);
  while (clutter_actor_iter_next (&iter, &child))
  {
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
  ClutterActorIter iter;
  ClutterActor *child;

  clutter_actor_iter_init (&iter, actor);
  while (clutter_actor_iter_next (&iter, &child))
  {
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
  ClutterActorIter iter;
  ClutterActor *child;

  clutter_actor_iter_init (&iter, actor);
  while (clutter_actor_iter_next (&iter, &child))
  {
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
  MxPadding padding = { 0, };
  gfloat height = 0, child_nat_h;
  ClutterActorIter iter;
  ClutterActor *child;

  mx_widget_get_padding (MX_WIDGET (box), &padding);
  height += padding.top + padding.bottom;

  /* Subtract padding */
  for_width -= (padding.left + padding.right);

  if (clutter_actor_get_n_children (CLUTTER_ACTOR (box)) > 1)
    {
      clutter_actor_iter_init (&iter, CLUTTER_ACTOR (box));
      while (clutter_actor_iter_next (&iter, &child))
        {
          clutter_actor_get_preferred_height (child,
                                              for_width,
                                              NULL,
                                              &child_nat_h);

          height += child_nat_h + SPACING;
        }

      height -= SPACING;
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
