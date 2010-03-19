/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-toolbar-icon.c */
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <glib/gi18n.h>

#include "mnb-toolbar-clock.h"
#include "mnb-toolbar.h"

G_DEFINE_TYPE (MnbToolbarClock, mnb_toolbar_clock, MNB_TYPE_TOOLBAR_BUTTON)


#define MNB_TOOLBAR_CLOCK_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_TOOLBAR_CLOCK, MnbToolbarClockPrivate))

struct _MnbToolbarClockPrivate
{
  ClutterActor *time;
  ClutterActor *date;

  guint         timeout_id;

  gboolean disposed : 1;
};

static void
mnb_toolbar_clock_dispose (GObject *object)
{
  MnbToolbarClockPrivate *priv = MNB_TOOLBAR_CLOCK (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  if (priv->timeout_id)
    {
      g_source_remove (priv->timeout_id);
      priv->timeout_id = 0;
    }

  clutter_actor_destroy (priv->time);
  priv->time = NULL;

  clutter_actor_destroy (priv->date);
  priv->date = NULL;

  G_OBJECT_CLASS (mnb_toolbar_clock_parent_class)->dispose (object);
}

static gboolean
mnb_toolbar_clock_update_time_date (MnbToolbarClockPrivate *priv)
{
  time_t         t;
  struct tm     *tmp;
  char           time_str[64];

  if (priv->disposed)
    return FALSE;

  t = time (NULL);
  tmp = localtime (&t);
  if (tmp)
    /* translators: translate this to a suitable time format for your locale
     * showing only hours and minutes. For available format specifiers see
     * http://www.opengroup.org/onlinepubs/007908799/xsh/strftime.html
     */
    strftime (time_str, 64, _("%l:%M %P"), tmp);
  else
    snprintf (time_str, 64, "Time");
  mx_label_set_text (MX_LABEL (priv->time), time_str);

  if (tmp)
    /* translators: translate this to a suitable date format for your locale.
     * For availabe format specifiers see
     * http://www.opengroup.org/onlinepubs/007908799/xsh/strftime.html
     */
    strftime (time_str, 64, _("%B %e, %Y"), tmp);
  else
    snprintf (time_str, 64, "Date");
  mx_label_set_text (MX_LABEL (priv->date), time_str);

  if (!priv->timeout_id) {
    priv->timeout_id =
      g_timeout_add_seconds (60, (GSourceFunc) mnb_toolbar_clock_update_time_date,
                             priv);
    return FALSE;
  }
  return TRUE;
}

static void
mnb_toolbar_clock_constructed (GObject *self)
{
  MnbToolbarClockPrivate *priv = MNB_TOOLBAR_CLOCK (self)->priv;
  ClutterActor           *actor = CLUTTER_ACTOR (self);
  time_t interval = 60 - (time(NULL) % 60);

  if (G_OBJECT_CLASS (mnb_toolbar_clock_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_toolbar_clock_parent_class)->constructed (self);

  clutter_actor_push_internal (actor);

  /* create time and date labels */
  priv->time = mx_label_new ();
  clutter_actor_set_name (priv->time, "time-label");
  clutter_actor_set_parent (priv->time, actor);

  priv->date = mx_label_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (priv->date), "date-label");
  clutter_actor_set_parent (priv->date, actor);

  clutter_actor_pop_internal (actor);

  mnb_toolbar_clock_update_time_date (priv);

  if (!interval) {
    priv->timeout_id =
      g_timeout_add_seconds (60, (GSourceFunc) mnb_toolbar_clock_update_time_date,
                             priv);
  } else {
    priv->timeout_id = 0;
    g_timeout_add_seconds (interval, (GSourceFunc) mnb_toolbar_clock_update_time_date,
                           priv);
  }
}

static void
mnb_toolbar_clock_map (ClutterActor *actor)
{
  MnbToolbarClockPrivate *priv = MNB_TOOLBAR_CLOCK (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_toolbar_clock_parent_class)->map (actor);

  clutter_actor_map (priv->time);
  clutter_actor_map (priv->date);
}

static void
mnb_toolbar_clock_unmap (ClutterActor *actor)
{
  MnbToolbarClockPrivate *priv = MNB_TOOLBAR_CLOCK (actor)->priv;

  clutter_actor_unmap (priv->time);
  clutter_actor_unmap (priv->date);

  CLUTTER_ACTOR_CLASS (mnb_toolbar_clock_parent_class)->unmap (actor);
}

static void
mnb_toolbar_clock_allocate (ClutterActor          *actor,
                            const ClutterActorBox *box,
                            ClutterAllocationFlags flags)
{
  MnbToolbarClockPrivate *priv = MNB_TOOLBAR_CLOCK (actor)->priv;
  ClutterActorBox         childbox;
  MxPadding               padding = {0,};

  CLUTTER_ACTOR_CLASS (
             mnb_toolbar_clock_parent_class)->allocate (actor, box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  childbox.x1 = padding.left;
  childbox.y1 = padding.top;
  childbox.x2 = (gfloat)((gint)(box->x2 - box->x1 - padding.right));
  childbox.y2 = (gfloat)((gint)(box->y2 - box->y1 - padding.bottom));

  mx_allocate_align_fill (priv->time, &childbox,
                          MX_ALIGN_MIDDLE,
                          MX_ALIGN_START, FALSE, FALSE);

  clutter_actor_allocate (priv->time, &childbox, flags);

  childbox.x1 = padding.left;
  childbox.y1 = padding.top;
  childbox.x2 = (gfloat)((gint)(box->x2 - box->x1 - padding.right));
  childbox.y2 = (gfloat)((gint)(box->y2 - box->y1 - padding.bottom));

  mx_allocate_align_fill (priv->date, &childbox,
                          MX_ALIGN_MIDDLE,
                          MX_ALIGN_END, FALSE, FALSE);

  clutter_actor_allocate (priv->date, &childbox, flags);
}

static void
mnb_toolbar_clock_paint (ClutterActor *self)
{
  MnbToolbarClockPrivate *priv = MNB_TOOLBAR_CLOCK (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_toolbar_clock_parent_class)->paint (self);

  clutter_actor_paint (priv->time);
  clutter_actor_paint (priv->date);
}

static void
mnb_toolbar_clock_get_preferred_width (ClutterActor *self,
                                       gfloat        for_height,
                                       gfloat       *min_width_p,
                                       gfloat       *natural_width_p)
{
  gfloat width;

  width = 164.0;

  if (min_width_p)
    *min_width_p = width;

  if (natural_width_p)
    *natural_width_p = width;
}

static void
mnb_toolbar_clock_get_preferred_height (ClutterActor *self,
                                        gfloat        for_width,
                                        gfloat       *min_height_p,
                                        gfloat       *natural_height_p)
{
  gfloat height;

  height = 55.0;

  if (min_height_p)
    *min_height_p = height;

  if (natural_height_p)
    *natural_height_p = height;
}

static void
mnb_toolbar_clock_class_init (MnbToolbarClockClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbToolbarClockPrivate));

  object_class->dispose             = mnb_toolbar_clock_dispose;
  object_class->constructed         = mnb_toolbar_clock_constructed;

  actor_class->allocate             = mnb_toolbar_clock_allocate;
  actor_class->map                  = mnb_toolbar_clock_map;
  actor_class->unmap                = mnb_toolbar_clock_unmap;
  actor_class->get_preferred_width  = mnb_toolbar_clock_get_preferred_width;
  actor_class->get_preferred_height = mnb_toolbar_clock_get_preferred_height;
  actor_class->paint                = mnb_toolbar_clock_paint;
}

static void
mnb_toolbar_clock_init (MnbToolbarClock *self)
{
  self->priv = MNB_TOOLBAR_CLOCK_GET_PRIVATE (self);
}

ClutterActor*
mnb_toolbar_clock_new (void)
{
  return g_object_new (MNB_TYPE_TOOLBAR_CLOCK, NULL);
}

