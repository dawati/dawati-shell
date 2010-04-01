/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mtp-toolbar-button.c */
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

#include <string.h>

#include "mtp-space.h"
#include "mtp-jar.h"

G_DEFINE_TYPE (MtpSpace, mtp_space, MX_TYPE_WIDGET);

#define MTP_SPACE_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MTP_TYPE_SPACE, MtpSpacePrivate))

enum
{
  PROP_0 = 0,

};

struct _MtpSpacePrivate
{
  gboolean applet   : 1;
  gboolean no_pick  : 1;
  gboolean in_jar   : 1;
  gboolean disposed : 1;
};

static void
mtp_space_dispose (GObject *object)
{
  MtpSpacePrivate *priv = MTP_SPACE (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  G_OBJECT_CLASS (mtp_space_parent_class)->dispose (object);
}

static void
mtp_space_finalize (GObject *object)
{
  /* MtpSpacePrivate *priv = MTP_SPACE (object)->priv; */

  G_OBJECT_CLASS (mtp_space_parent_class)->finalize (object);
}

static void
mtp_space_get_preferred_width (ClutterActor *self,
                               gfloat        for_height,
                               gfloat       *min_width_p,
                               gfloat       *natural_width_p)
{
  MtpSpacePrivate *priv = MTP_SPACE (self)->priv;
  gfloat           width;

  width = priv->in_jar ? 200.0 : (priv->applet ? 40.0 : 60.0);

  if (min_width_p)
    *min_width_p = width;

  if (natural_width_p)
    *natural_width_p = width;
}

static void
mtp_space_get_preferred_height (ClutterActor *self,
                                gfloat        for_width,
                                gfloat       *min_height_p,
                                gfloat       *natural_height_p)
{
  gfloat                   height;
  MxPadding                padding;

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  height = 64.0 + (padding.top + padding.bottom);

  if (min_height_p)
    *min_height_p = height;

  if (natural_height_p)
    *natural_height_p = height;
}

static void
mtp_space_allocate (ClutterActor          *actor,
                    const ClutterActorBox *box,
                    ClutterAllocationFlags flags)
{
  CLUTTER_ACTOR_CLASS (
             mtp_space_parent_class)->allocate (actor,
                                                box,
                                                flags);
}

static void
mtp_space_pick (ClutterActor *self, const ClutterColor *color)
{
  MtpSpacePrivate *priv = MTP_SPACE (self)->priv;

  if (priv->no_pick)
    return;

  CLUTTER_ACTOR_CLASS (mtp_space_parent_class)->pick (self, color);
}

static void
mtp_space_paint (ClutterActor *self)
{
  MtpSpacePrivate *priv = MTP_SPACE (self)->priv;

  /*
   * The spaces in the applet area are invisible.
   */
  if (priv->applet)
    return;

  CLUTTER_ACTOR_CLASS (mtp_space_parent_class)->paint (self);
}

static void
mtp_space_parent_set (ClutterActor *space, ClutterActor *old_parent)
{
  MtpSpacePrivate   *priv = MTP_SPACE (space)->priv;
  ClutterActor      *stables;
  ClutterActorClass *klass;

  klass = CLUTTER_ACTOR_CLASS (mtp_space_parent_class);

  if (klass->parent_set)
    klass->parent_set (space, old_parent);

  stables = clutter_actor_get_parent (space);

  if (!stables)
    return;

  while (stables && !MTP_IS_JAR (stables))
    stables = clutter_actor_get_parent (stables);

  if (!stables || !MTP_IS_JAR (stables))
    {
      priv->in_jar = FALSE;
      clutter_actor_set_name (space, NULL);
    }
  else
    {
      priv->in_jar   = TRUE;
      clutter_actor_set_name (space, "in-jar");
    }
}

static void
mtp_space_class_init (MtpSpaceClass *klass)
{
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MtpSpacePrivate));

  actor_class->allocate             = mtp_space_allocate;
  actor_class->get_preferred_width  = mtp_space_get_preferred_width;
  actor_class->get_preferred_height = mtp_space_get_preferred_height;
  actor_class->pick                 = mtp_space_pick;
  actor_class->paint                = mtp_space_paint;
  actor_class->parent_set           = mtp_space_parent_set;

  object_class->dispose             = mtp_space_dispose;
  object_class->finalize            = mtp_space_finalize;
}

static void
mtp_space_init (MtpSpace *self)
{
  self->priv = MTP_SPACE_GET_PRIVATE (self);
}

ClutterActor*
mtp_space_new (void)
{
  return g_object_new (MTP_TYPE_SPACE, NULL);
}

void
mtp_space_set_dont_pick (MtpSpace *button, gboolean dont)
{
  MtpSpacePrivate *priv = MTP_SPACE (button)->priv;

  priv->no_pick = dont;
}

void
mtp_space_set_is_applet (MtpSpace *button, gboolean applet)
{
  MtpSpacePrivate *priv = MTP_SPACE (button)->priv;

  priv->applet = applet;
}
