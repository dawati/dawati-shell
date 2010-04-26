/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mtp--clock.c */
/*
 * Copyright (c) 2009, 2010 Intel Corp.
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

#include <config.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <gconf/gconf-client.h>

#include "mtp-clock.h"

#define MNB_24H_KEY "/apps/date-time-panel/24_h_clock"

/*
 * MtpClock
 */
G_DEFINE_TYPE (MtpClock, mtp_clock, MX_TYPE_WIDGET);

#define MTP_CLOCK_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MTP_TYPE_CLOCK, MtpClockPrivate))

enum
{
  PROP_0 = 0,

};

struct _MtpClockPrivate
{
  ClutterActor *time;

  gboolean disposed : 1;
};

static void
mtp_clock_dispose (GObject *object)
{
  MtpClockPrivate *priv = MTP_CLOCK (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  clutter_actor_destroy (priv->time);
  priv->time = NULL;

  G_OBJECT_CLASS (mtp_clock_parent_class)->dispose (object);
}

static void
mtp_clock_map (ClutterActor *actor)
{
  MtpClockPrivate *priv = MTP_CLOCK (actor)->priv;

  CLUTTER_ACTOR_CLASS (mtp_clock_parent_class)->map (actor);

  clutter_actor_map (priv->time);
}

static void
mtp_clock_unmap (ClutterActor *actor)
{
  MtpClockPrivate *priv = MTP_CLOCK (actor)->priv;

  clutter_actor_unmap (priv->time);

  CLUTTER_ACTOR_CLASS (mtp_clock_parent_class)->unmap (actor);
}

static void
mtp_clock_allocate (ClutterActor          *actor,
                    const ClutterActorBox *box,
                    ClutterAllocationFlags flags)
{
  MtpClockPrivate *priv = MTP_CLOCK (actor)->priv;
  ClutterActorBox  childbox;
  MxPadding        padding = {0,};
  ClutterActor    *background;

  CLUTTER_ACTOR_CLASS (
             mtp_clock_parent_class)->allocate (actor, box, flags);

  /*
   * Mx does not cope well with the background when the natural size is bigger
   * than the real size. Allocate the clock background properly.
   */
  if ((background = mx_widget_get_background_image (MX_WIDGET (actor))))
    {
      ClutterActorBox frame_box = {0, 0, box->x2 - box->x1, box->y2 - box->y1};

      clutter_actor_allocate (background, &frame_box, flags);
    }

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  childbox.x1 = padding.left;
  childbox.y1 = padding.top;
  childbox.x2 = (gfloat)((gint)(box->x2 - box->x1 - padding.right));
  childbox.y2 = (gfloat)((gint)(box->y2 - box->y1 - padding.bottom));

  mx_allocate_align_fill (priv->time, &childbox,
                          MX_ALIGN_MIDDLE,
                          MX_ALIGN_MIDDLE, FALSE, FALSE);

  clutter_actor_allocate (priv->time, &childbox, flags);
}


static void
mtp_clock_get_preferred_width (ClutterActor *self,
                               gfloat        for_height,
                               gfloat       *min_width_p,
                               gfloat       *natural_width_p)
{
  gfloat width;

  width = 120.0;

  if (min_width_p)
    *min_width_p = width;

  if (natural_width_p)
    *natural_width_p = width;
}

static void
mtp_clock_get_preferred_height (ClutterActor *self,
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
mtp_clock_paint (ClutterActor *self)
{
  MtpClockPrivate *priv = MTP_CLOCK (self)->priv;

  CLUTTER_ACTOR_CLASS (mtp_clock_parent_class)->paint (self);

  clutter_actor_paint (priv->time);
}

static void
mtp_clock_class_init (MtpClockClass *klass)
{
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MtpClockPrivate));

  actor_class->allocate             = mtp_clock_allocate;
  actor_class->map                  = mtp_clock_map;
  actor_class->unmap                = mtp_clock_unmap;
  actor_class->get_preferred_width  = mtp_clock_get_preferred_width;
  actor_class->get_preferred_height = mtp_clock_get_preferred_height;
  actor_class->paint                = mtp_clock_paint;

  object_class->dispose             = mtp_clock_dispose;
}

static void
mtp_clock_update_time_date (MtpClock *clock)
{
  MtpClockPrivate *priv = clock->priv;
  time_t           t;
  struct tm       *tmp;
  char             time_str[64];
  char             *time_ptr;

  t = time (NULL);
  tmp = localtime (&t);

  if (tmp)
    {
      GConfClient* client = gconf_client_get_default ();
      gboolean     c24h   = gconf_client_get_bool (client, MNB_24H_KEY, NULL);

      if (c24h)
        {

          /* translators: translate this to a suitable 24 hourt time format for
           * your locale showing only hours and minutes. For available format
           * specifiers see
           * http://www.opengroup.org/onlinepubs/007908799/xsh/strftime.html
           */
          strftime (time_str, 64, _("%H:%M"), tmp);
        }
      else
        {

          /* translators: translate this to a suitable time format for your
           * locale showing only hours and minutes. For available format
           * specifiers see
           * http://www.opengroup.org/onlinepubs/007908799/xsh/strftime.html
           */
          strftime (time_str, 64, _("%l:%M %P"), tmp);
        }
    }
  else
    snprintf (time_str, 64, "Time");

  /*
   * Strip leading space
   */
  if (time_str[0] == ' ')
    time_ptr = &time_str[1];
  else
    time_ptr = &time_str[0];

  mx_label_set_text (MX_LABEL (priv->time), time_ptr);

#if 0
  if (tmp)
    /* translators: translate this to a suitable date format for your locale.
     * For availabe format specifiers see
     * http://www.opengroup.org/onlinepubs/007908799/xsh/strftime.html
     */
    strftime (time_str, 64, _("%B %e, %Y"), tmp);
  else
    snprintf (time_str, 64, "Date");

  mx_widget_set_tooltip_text (MX_WIDGET (clock), time_str);
#endif
}

static void
mtp_clock_init (MtpClock *self)
{
  MtpClockPrivate *priv;

  priv = self->priv = MTP_CLOCK_GET_PRIVATE (self);

  /* create time and date labels */
  priv->time = mx_label_new ();
  clutter_actor_set_name (priv->time, "time-label");
  clutter_actor_set_parent (priv->time, (ClutterActor*)self);

  mtp_clock_update_time_date (self);
}

ClutterActor*
mtp_clock_new (void)
{
  return g_object_new (MTP_TYPE_CLOCK, NULL);
}
