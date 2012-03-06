/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

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

#include "mpl-panel-background.h"

G_DEFINE_TYPE (MplPanelBackground, mpl_panel_background, MX_TYPE_WIDGET);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_PANEL_BACKGROUND, MplPanelBackgroundPrivate))

enum {
  PROP_0,
};

struct _MplPanelBackgroundPrivate
{
  gulong        stage_alloc_cb;
  ClutterActor *stage;

  gboolean disposed        : 1;
  gboolean base_geom_known : 1;

  gfloat base_t, base_r, base_b, base_l;
};

static void
mpl_panel_background_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_panel_background_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_panel_background_dispose (GObject *object)
{
  MplPanelBackgroundPrivate *priv = MPL_PANEL_BACKGROUND (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  if (priv->stage_alloc_cb)
    {
      g_signal_handler_disconnect (priv->stage, priv->stage_alloc_cb);
      priv->stage_alloc_cb = 0;
      priv->stage = NULL;
    }

  G_OBJECT_CLASS (mpl_panel_background_parent_class)->dispose (object);
}

static void
mpl_panel_background_finalize (GObject *object)
{
  G_OBJECT_CLASS (mpl_panel_background_parent_class)->finalize (object);
}

/*
 * Track size of stage
 */
static void
mpl_panel_background_stage_allocation_cb (ClutterActor *stage,
                                          GParamSpec   *spec,
                                          ClutterActor *background)
{
  gfloat width, height, swidth, sheight;

  clutter_actor_get_size (stage, &swidth, &sheight);
  clutter_actor_get_size (background, &width, &height);

  if (width != swidth || height != sheight)
    clutter_actor_queue_relayout (background);
}

static void
mpl_panel_background_get_preferred_width (ClutterActor *self,
                                          gfloat        for_height,
                                          gfloat       *min_width_p,
                                          gfloat       *natural_width_p)
{
  MplPanelBackgroundPrivate *priv = MPL_PANEL_BACKGROUND (self)->priv;
  ClutterActor              *stage = clutter_actor_get_stage (self);
  gfloat                     width = 0.0;

  g_return_if_fail (stage);

  /*
   * This is as good place to hook the ClutterStage::allocation callback as any.
   */
  if (!priv->stage_alloc_cb)
    {
      priv->stage = stage;

      priv->stage_alloc_cb =
        g_signal_connect (stage, "notify::allocation",
                          G_CALLBACK (mpl_panel_background_stage_allocation_cb),
                          self);
    }

  width = clutter_actor_get_width (stage);

  if (min_width_p)
    *min_width_p = width;

  if (natural_width_p)
    *natural_width_p = width;
}

static void
mpl_panel_background_get_preferred_height (ClutterActor *self,
                                           gfloat        for_width,
                                           gfloat       *min_height_p,
                                           gfloat       *natural_height_p)
{
  MplPanelBackgroundPrivate *priv = MPL_PANEL_BACKGROUND (self)->priv;
  ClutterActor              *stage = clutter_actor_get_stage (self);
  gfloat                     height = 0.0;

  /*
   * This is as good place to hook the ClutterStage::allocation callback as any.
   */
  if (!priv->stage_alloc_cb)
    {
      priv->stage = stage;

      priv->stage_alloc_cb =
        g_signal_connect (stage, "notify::allocation",
                          G_CALLBACK (mpl_panel_background_stage_allocation_cb),
                          self);
    }

  height = clutter_actor_get_height (stage);

  if (min_height_p)
    *min_height_p = height;

  if (natural_height_p)
    *natural_height_p = height;
}

static void
mpl_panel_background_class_init (MplPanelBackgroundClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MplPanelBackgroundPrivate));

  object_class->get_property        = mpl_panel_background_get_property;
  object_class->set_property        = mpl_panel_background_set_property;
  object_class->dispose             = mpl_panel_background_dispose;
  object_class->finalize            = mpl_panel_background_finalize;

  actor_class->get_preferred_width  = mpl_panel_background_get_preferred_width;
  actor_class->get_preferred_height = mpl_panel_background_get_preferred_height;
}

static void
mpl_panel_background_init (MplPanelBackground *self)
{
  self->priv = GET_PRIVATE (self);
}

MplPanelBackground*
mpl_panel_background_new (void)
{
  return g_object_new (MPL_TYPE_PANEL_BACKGROUND, NULL);
}

