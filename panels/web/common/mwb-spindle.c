/*
 * Moblin-Web-Browser: The web browser for Moblin
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter/clutter.h>
#include <math.h>

#include "mwb-spindle.h"

/*
 * MwbSpindle is a ClutterContainer that displays one child at a
 * time. You can use the 'position' property to animate switching
 * between between children. When the position is between actors they
 * will be rotated to appear as if they are on a spindle along the X
 * axis.
 *
 * The preferred size of the container will be the preferred size of
 * the largest child. All children are allocated the same size. The
 * children need to be able to cope with being painted while backface
 * culling is enabled to work properly.
 */

static void mwb_spindle_dispose (GObject *object);

static void clutter_container_iface_init (ClutterContainerIface *iface);

G_DEFINE_TYPE_WITH_CODE (MwbSpindle, mwb_spindle, CLUTTER_TYPE_ACTOR,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init));


#define MWB_SPINDLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MWB_TYPE_SPINDLE, \
                                MwbSpindlePrivate))

struct _MwbSpindlePrivate
{
  GSList *children;
  gdouble position;
};

enum
{
  PROP_0,

  PROP_POSITION
};

static void
mwb_spindle_real_add (ClutterContainer *container,
                      ClutterActor *actor)
{
  MwbSpindle *spindle = MWB_SPINDLE (container);
  MwbSpindlePrivate *priv = spindle->priv;

  g_object_ref (actor);

  priv->children = g_slist_append (priv->children, actor);
  clutter_actor_set_parent (actor, CLUTTER_ACTOR (spindle));

  clutter_actor_queue_relayout (CLUTTER_ACTOR (spindle));

  g_signal_emit_by_name (container, "actor-added", actor);

  g_object_unref (actor);
}

static void
mwb_spindle_real_remove (ClutterContainer *container,
                         ClutterActor *actor)
{
  MwbSpindle *spindle = MWB_SPINDLE (container);
  MwbSpindlePrivate *priv = spindle->priv;

  g_object_ref (actor);

  priv->children = g_slist_remove (priv->children, actor);
  clutter_actor_unparent (actor);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (spindle));

  g_signal_emit_by_name (container, "actor-removed", actor);

  g_object_unref (actor);
}

static void
mwb_spindle_real_foreach (ClutterContainer *container,
                          ClutterCallback callback,
                          gpointer user_data)
{
  MwbSpindle *spindle = MWB_SPINDLE (container);
  MwbSpindlePrivate *priv = spindle->priv;

  g_slist_foreach (priv->children, (GFunc) callback, user_data);
}

static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
  iface->add = mwb_spindle_real_add;
  iface->remove = mwb_spindle_real_remove;
  iface->foreach = mwb_spindle_real_foreach;
}

static void
mwb_spindle_paint (ClutterActor *actor)
{
  MwbSpindlePrivate *priv = MWB_SPINDLE (actor)->priv;
  gint int_position = rint (priv->position);

  /* If the position is near enough to a whole number then just paint
     one of the children directly */
  if (fabs (int_position - priv->position) < 1e-5)
    {
      ClutterActor *child = g_slist_nth_data (priv->children, int_position);

      if (child)
        clutter_actor_paint (child);
    }
  else
    {
      ClutterGeometry geom;
      gdouble integer_part, fractional_part;
      GSList *child_pos;
      guint n_children;

      clutter_actor_get_allocation_geometry (actor, &geom);
      n_children = g_slist_length (priv->children);

      /* There should be at most two children visible so we don't need
         to paint all of them. Instead we round down the position to
         the next integer and draw that along with the next actor
         rotated according to the fractional part */
      fractional_part = modf (priv->position, &integer_part);
      child_pos = g_slist_nth (priv->children, integer_part);

      if (child_pos)
        {
          gfloat apothem;
          gboolean was_backface_culling_enabled =
            cogl_get_backface_culling_enabled ();
          cogl_set_backface_culling_enabled (TRUE);

          /* The 'apothem' is the distance from the center of the
             regular polygon to the midpoint of one of the sides. We
             need to know this to calculate a rotation center which
             whould make the edges of the children join up. The
             formula for this is taken from:
             http://www.mathopenref.com/polygonapothem.html */
          if (n_children < 3)
            apothem = 0.0f;
          else
            apothem = geom.height / (2 * tanf (G_PI / n_children));

          /* We need to clip to the allocation for the animation to
             work */
          cogl_clip_push (0, 0, geom.width, geom.height);

          cogl_push_matrix ();
          cogl_translate (0.0f, geom.height / 2.0f, -apothem);
          cogl_rotate (360.0f / n_children * fractional_part,
                       1.0f, 0.0f, 0.0f);
          cogl_translate (0.0f, geom.height / -2.0f, apothem);
          clutter_actor_paint (child_pos->data);
          cogl_pop_matrix ();

          /* Also paint the next child if there is one */
          if (child_pos->next)
            {
              cogl_push_matrix ();
              cogl_translate (0.0f, geom.height / 2.0f, -apothem);
              cogl_rotate (360.0f / n_children * (fractional_part - 1.0f),
                           1.0f, 0.0f, 0.0f);
              cogl_translate (0.0f, geom.height / -2.0f, apothem);
              clutter_actor_paint (child_pos->next->data);
              cogl_pop_matrix ();
            }

          cogl_clip_pop ();

          cogl_set_backface_culling_enabled (was_backface_culling_enabled);
        }
    }
}

static void
mwb_spindle_pick (ClutterActor *actor,
                  const ClutterColor *color)
{
  /* Chain up so we get a bounding box pained (if we are reactive) */
  CLUTTER_ACTOR_CLASS (mwb_spindle_parent_class)->pick (actor, color);

  mwb_spindle_paint (actor);
}

static void
mwb_spindle_get_preferred_width (ClutterActor *self,
                                 gfloat for_height,
                                 gfloat *min_width_p,
                                 gfloat *natural_width_p)
{
  MwbSpindlePrivate *priv = MWB_SPINDLE (self)->priv;
  GSList *l;
  gfloat min_width = 0.0f, natural_width = 0.0f;

  /* Use the size of the largest child */
  for (l = priv->children; l; l = l->next)
    {
      gfloat child_min_width, child_natural_width;

      clutter_actor_get_preferred_width (l->data,
                                         for_height,
                                         &child_min_width,
                                         &child_natural_width);

      if (child_min_width > min_width)
        min_width = child_min_width;
      if (child_natural_width > natural_width)
        natural_width = child_natural_width;
    }

  if (min_width_p)
    *min_width_p = min_width;
  if (natural_width_p)
    *natural_width_p = natural_width;
}

static void
mwb_spindle_get_preferred_height (ClutterActor *self,
                                  gfloat for_width,
                                  gfloat *min_height_p,
                                  gfloat *natural_height_p)
{
  MwbSpindlePrivate *priv = MWB_SPINDLE (self)->priv;
  GSList *l;
  gfloat min_height = 0.0f, natural_height = 0.0f;

  /* Use the size of the largest child */
  for (l = priv->children; l; l = l->next)
    {
      gfloat child_min_height, child_natural_height;

      clutter_actor_get_preferred_height (l->data,
                                          for_width,
                                          &child_min_height,
                                          &child_natural_height);

      if (child_min_height > min_height)
        min_height = child_min_height;
      if (child_natural_height > natural_height)
        natural_height = child_natural_height;
    }

  if (min_height_p)
    *min_height_p = min_height;
  if (natural_height_p)
    *natural_height_p = natural_height;
}

static void
mwb_spindle_allocate (ClutterActor *self,
                      const ClutterActorBox *box,
                      ClutterAllocationFlags flags)
{
  MwbSpindlePrivate *priv = MWB_SPINDLE (self)->priv;
  ClutterActorBox child_allocation;
  GSList *l;

  /* chain up to set actor->allocation */
  CLUTTER_ACTOR_CLASS (mwb_spindle_parent_class)->allocate (self, box, flags);

  /* Allocate all of the children with the same size */
  child_allocation.x1 = 0.0f;
  child_allocation.y1 = 0.0f;
  child_allocation.x2 = box->x2 - box->x1;
  child_allocation.y2 = box->y2 - box->y1;

  for (l = priv->children; l; l = l->next)
    clutter_actor_allocate (l->data, &child_allocation, flags);
}

static void
mwb_spindle_show_all (ClutterActor *actor)
{
  clutter_container_foreach (CLUTTER_CONTAINER (actor),
                             CLUTTER_CALLBACK (clutter_actor_show),
                             NULL);
  clutter_actor_show (actor);
}

static void
mwb_spindle_hide_all (ClutterActor *actor)
{
  clutter_actor_hide (actor);
  clutter_container_foreach (CLUTTER_CONTAINER (actor),
                             CLUTTER_CALLBACK (clutter_actor_hide),
                             NULL);
}

static void
mwb_spindle_get_property (GObject *object,
                          guint prop_id,
                          GValue *value,
                          GParamSpec *pspec)
{
  MwbSpindle *spindle = MWB_SPINDLE (object);

  switch (prop_id)
    {
    case PROP_POSITION:
      g_value_set_double (value, mwb_spindle_get_position (spindle));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
mwb_spindle_set_property (GObject *object,
                          guint prop_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
  MwbSpindle *spindle = MWB_SPINDLE (object);

  switch (prop_id)
    {
    case PROP_POSITION:
      mwb_spindle_set_position (spindle, g_value_get_double (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
mwb_spindle_class_init (MwbSpindleClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  gobject_class->dispose = mwb_spindle_dispose;
  gobject_class->get_property = mwb_spindle_get_property;
  gobject_class->set_property = mwb_spindle_set_property;

  actor_class->paint = mwb_spindle_paint;
  actor_class->pick = mwb_spindle_pick;
  actor_class->show_all = mwb_spindle_show_all;
  actor_class->hide_all = mwb_spindle_hide_all;

  actor_class->get_preferred_width = mwb_spindle_get_preferred_width;
  actor_class->get_preferred_height = mwb_spindle_get_preferred_height;
  actor_class->allocate = mwb_spindle_allocate;

  pspec = g_param_spec_double ("position",
                               "Position",
                               "A number to select which child to display. "
                               "A fractional number can be used to make the "
                               "spindle to appear rotated in-between two "
                               "children.",
                               -G_MAXDOUBLE, G_MAXDOUBLE,
                               0.0,
                               G_PARAM_READWRITE |
                               G_PARAM_STATIC_NAME |
                               G_PARAM_STATIC_NICK |
                               G_PARAM_STATIC_BLURB);
  g_object_class_install_property (gobject_class, PROP_POSITION, pspec);

  g_type_class_add_private (klass, sizeof (MwbSpindlePrivate));
}

static void
mwb_spindle_init (MwbSpindle *self)
{
  self->priv = MWB_SPINDLE_GET_PRIVATE (self);
}

static void
mwb_spindle_dispose (GObject *object)
{
  MwbSpindle *self = (MwbSpindle *) object;
  MwbSpindlePrivate *priv = self->priv;

  g_slist_foreach (priv->children, (GFunc) clutter_actor_destroy, NULL);
  g_slist_free (priv->children);
  priv->children = NULL;

  G_OBJECT_CLASS (mwb_spindle_parent_class)->dispose (object);
}

ClutterActor *
mwb_spindle_new (void)
{
  ClutterActor *self = g_object_new (MWB_TYPE_SPINDLE, NULL);

  return self;
}

void
mwb_spindle_set_position (MwbSpindle *spindle, gdouble position)
{
  MwbSpindlePrivate *priv;

  g_return_if_fail (MWB_IS_SPINDLE (spindle));

  priv = spindle->priv;

  priv->position = position;

  clutter_actor_queue_redraw (CLUTTER_ACTOR (spindle));

  g_object_notify (G_OBJECT (spindle), "position");
}

gdouble
mwb_spindle_get_position (MwbSpindle *spindle)
{
  g_return_val_if_fail (MWB_IS_SPINDLE (spindle), 0.0);

  return spindle->priv->position;
}

