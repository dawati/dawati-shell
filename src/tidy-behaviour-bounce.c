/*
 * Tidy.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2006 OpenedHand
 *
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

/**
 * SECTION:tidy-behaviour-bounce
 * @short_description: A behaviour controlling bounce
 *
 * A #TidyBehaviourBounce interpolates actors size between two values.
 *
 */

#include "tidy-behaviour-bounce.h"
#include <math.h>

G_DEFINE_TYPE (TidyBehaviourBounce,   \
               tidy_behaviour_bounce, \
	       CLUTTER_TYPE_BEHAVIOUR);

struct _TidyBehaviourBouncePrivate
{
  gint foo;
};

#define TIDY_BEHAVIOUR_BOUNCE_GET_PRIVATE(obj)        \
              (G_TYPE_INSTANCE_GET_PRIVATE ((obj),    \
               TIDY_TYPE_BEHAVIOUR_BOUNCE,            \
               TidyBehaviourBouncePrivate))

typedef struct {
  ClutterFixed scale;
  guint8       opacity;
} BounceFrameClosure;

static void
bounce_frame_foreach (ClutterBehaviour *behaviour,
                      ClutterActor     *actor,
                      gpointer          data)
{
  BounceFrameClosure *closure = data;

  clutter_actor_set_opacity (actor, closure->opacity);
  clutter_actor_set_scalex (actor, closure->scale, closure->scale);
}

static void
tidy_behaviour_bounce_alpha_notify (ClutterBehaviour *behave,
                                    guint32           alpha_value)
{
  TidyBehaviourBouncePrivate *priv;
  guint boing_alpha;                    
  ClutterFixed factor;

  BounceFrameClosure closure = { 0, };

  priv = TIDY_BEHAVIOUR_BOUNCE (behave)->priv;

  boing_alpha = (CLUTTER_ALPHA_MAX_ALPHA/4)*3;

  if (alpha_value < boing_alpha)
    {
      factor = CLUTTER_INT_TO_FIXED (alpha_value) / boing_alpha;

      closure.scale = factor + CLUTTER_FLOAT_TO_FIXED(0.25);
      closure.opacity = CLUTTER_FIXED_TO_INT(0xff * factor); 
    }
  else 
    {
      closure.opacity = 0xff;

      /* scale down from 1.25 -> 1.0 */
      
      factor = CLUTTER_INT_TO_FIXED (alpha_value - boing_alpha) 
                / (CLUTTER_ALPHA_MAX_ALPHA  - boing_alpha);

      factor /= 4;

      closure.scale = CLUTTER_FLOAT_TO_FIXED(1.25) - factor;
    }

  clutter_behaviour_actors_foreach (behave,
                                    bounce_frame_foreach,
                                    &closure);
}

static void
tidy_behaviour_bounce_class_init (TidyBehaviourBounceClass *klass)
{
  ClutterBehaviourClass *behave_class = CLUTTER_BEHAVIOUR_CLASS (klass);

  behave_class->alpha_notify = tidy_behaviour_bounce_alpha_notify;

  g_type_class_add_private (klass, sizeof (TidyBehaviourBouncePrivate));
}

static void
tidy_behaviour_bounce_init (TidyBehaviourBounce *self)
{
  TidyBehaviourBouncePrivate *priv;

  self->priv = priv = TIDY_BEHAVIOUR_BOUNCE_GET_PRIVATE (self);
}

/**
 * tidy_behaviour_bounce_newx:
 * @alpha: a #TidyAlpha
 * @x_bounce_start: initial bounce factor on the X axis
 * @y_bounce_start: initial bounce factor on the Y axis
 * @x_bounce_end: final bounce factor on the X axis
 * @y_bounce_end: final bounce factor on the Y axis
 *
 * A fixed point implementation of tidy_behaviour_bounce_new()
 *
 * Return value: the newly created #TidyBehaviourBounce
 *
 * Since: 0.2
 */
ClutterBehaviour *
tidy_behaviour_bounce_new (ClutterAlpha   *alpha)
{
  TidyBehaviourBounce *behave;

  g_return_val_if_fail (alpha == NULL || CLUTTER_IS_ALPHA (alpha), NULL);

  behave = g_object_new (TIDY_TYPE_BEHAVIOUR_BOUNCE, "alpha", alpha, NULL);

  return CLUTTER_BEHAVIOUR (behave);
}


typedef struct BounceClosure
{
  ClutterActor             *actor;
  ClutterTimeline          *timeline;
  ClutterAlpha             *alpha;
  ClutterBehaviour         *behave;
  gulong                    signal_id;
}
BounceClosure;

static void
on_bounce_complete (ClutterTimeline *timeline,
		    gpointer         user_data)
{
  BounceClosure *b =  (BounceClosure*)user_data;

  g_signal_handler_disconnect (b->timeline, b->signal_id);

  g_object_unref (b->actor);
  g_object_unref (b->behave);
  g_object_unref (b->timeline);

  g_slice_free (BounceClosure, b);
}

ClutterTimeline*
tidy_bounce_scale (ClutterActor *actor, gint duration)
{
  BounceClosure *b;

  b = g_slice_new0(BounceClosure);

  g_object_ref (actor);

  b->actor    = actor;
  b->timeline = clutter_timeline_new_for_duration (duration);  
  b->signal_id = g_signal_connect (b->timeline, "completed", 
                                   G_CALLBACK (on_bounce_complete), b);

  b->alpha = clutter_alpha_new_full (b->timeline, CLUTTER_ALPHA_SINE_INC, 
                                     NULL, NULL);
  b->behave = tidy_behaviour_bounce_new (b->alpha);

  clutter_behaviour_apply (b->behave, b->actor);

  clutter_timeline_start (b->timeline);

  return b->timeline;
}                   



