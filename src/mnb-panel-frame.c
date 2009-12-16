/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009 Intel Corp.
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

#include "mnb-panel-frame.h"

G_DEFINE_TYPE (MnbPanelFrame, mnb_panel_frame, NBTK_TYPE_WIDGET);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_PANEL_FRAME, MnbPanelFramePrivate))

enum {
  PROP_0,
};

enum
{
  FOOTER_CLICKED,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _MnbPanelFramePrivate
{

  ClutterActor *footer;

  gboolean disposed : 1;
};

static void
mnb_panel_frame_get_property (GObject    *object,
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
mnb_panel_frame_set_property (GObject      *object,
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
mnb_panel_frame_dispose (GObject *object)
{
  MnbPanelFramePrivate *priv = MNB_PANEL_FRAME (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  if (priv->footer)
    {
      clutter_actor_destroy (priv->footer);
      priv->footer = NULL;
    }

  G_OBJECT_CLASS (mnb_panel_frame_parent_class)->dispose (object);
}

static void
mnb_panel_frame_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_panel_frame_parent_class)->finalize (object);
}

#if 0
static void
mnb_panel_frame_footer_clicked_cb (NbtkButton *button, MnbPanelFrame *frame)
{
  g_object_ref (frame);
  g_signal_emit (frame, signals[FOOTER_CLICKED], 0);
  g_object_unref (frame);
}
#endif

static void
mnb_panel_frame_constructed (GObject *object)
{
#if 0
  MnbPanelFramePrivate *priv = MNB_PANEL_FRAME (object)->priv;
  NbtkWidget         *footer;

  /* footer with "up" button */
  footer = nbtk_button_new ();
  nbtk_widget_set_style_class_name (footer, "drop-down-footer");

  clutter_actor_set_parent ((ClutterActor*)footer, CLUTTER_ACTOR (object));

  g_signal_connect (footer, "clicked",
                    G_CALLBACK (mnb_panel_frame_footer_clicked_cb), object);

  priv->footer = CLUTTER_ACTOR (footer);
#endif
}

static void
mnb_panel_frame_allocate (ClutterActor          *self,
                          const ClutterActorBox *box,
                          ClutterAllocationFlags flags)
{

  CLUTTER_ACTOR_CLASS (mnb_panel_frame_parent_class)->allocate (self, box,
                                                                flags);

#if 0
  MnbPanelFramePrivate *priv = MNB_PANEL_FRAME (self)->priv;
  gfloat                min_height, natural_height;
  ClutterActorBox allocation = { 0, };
  NbtkPadding     padding = { 0, };

  nbtk_widget_get_padding (NBTK_WIDGET (self), &padding);

  clutter_actor_get_preferred_height (priv->footer, -1,
                                      &min_height,
                                      &natural_height);

  allocation.x1 = padding.left;
  allocation.x2 = box->x2 - box->x1 - padding.right;
  allocation.y1 = (box->y2 - box->y1) - (padding.bottom + natural_height);
  allocation.y2 = allocation.y1 + natural_height;

  clutter_actor_allocate (priv->footer, &allocation, flags);
#endif
}

static void
mnb_panel_frame_get_preferred_width (ClutterActor *self,
                                     gfloat        for_height,
                                     gfloat       *min_width_p,
                                     gfloat       *natural_width_p)
{
  gfloat      min_width, natural_width;
  gfloat      available_height;
  NbtkPadding padding = { 0, };

  nbtk_widget_get_padding (NBTK_WIDGET (self), &padding);

  available_height = for_height - padding.top - padding.bottom;

  min_width = natural_width = padding.left + padding.right;

  if (min_width_p)
    *min_width_p = min_width;

  if (natural_width_p)
    *natural_width_p = natural_width;
}

static void
mnb_panel_frame_get_preferred_height (ClutterActor *self,
                                      gfloat        for_width,
                                      gfloat       *min_height_p,
                                      gfloat       *natural_height_p)
{
  gfloat      min_height, natural_height;
  gfloat      available_width;
  NbtkPadding padding = { 0, };

  nbtk_widget_get_padding (NBTK_WIDGET (self), &padding);

  available_width = for_width - padding.left - padding.right;

  min_height = natural_height = padding.top + padding.bottom;

  if (min_height_p)
    *min_height_p = min_height;

  if (natural_height_p)
    *natural_height_p = natural_height;
}

static void
mnb_panel_frame_paint (ClutterActor *self)
{
  MnbPanelFramePrivate *priv = MNB_PANEL_FRAME (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_panel_frame_parent_class)->paint (self);

  if (priv->footer)
    clutter_actor_paint (priv->footer);
}

static void
mnb_panel_frame_pick (ClutterActor *self, const ClutterColor *pick_color)
{
  MnbPanelFramePrivate *priv = MNB_PANEL_FRAME (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_panel_frame_parent_class)->pick (self, pick_color);

  if (priv->footer)
    clutter_actor_paint (priv->footer);
}

static void
mnb_panel_frame_map (ClutterActor *self)
{
  MnbPanelFramePrivate *priv = MNB_PANEL_FRAME (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_panel_frame_parent_class)->map (self);

  if (priv->footer)
    clutter_actor_map (priv->footer);
}

static void
mnb_panel_frame_unmap (ClutterActor *self)
{
  MnbPanelFramePrivate *priv = MNB_PANEL_FRAME (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_panel_frame_parent_class)->unmap (self);

  if (priv->footer)
    clutter_actor_unmap (priv->footer);
}

static void
mnb_panel_frame_class_init (MnbPanelFrameClass *klass)
{
  GObjectClass      *object_class  = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbPanelFramePrivate));

  object_class->get_property = mnb_panel_frame_get_property;
  object_class->set_property = mnb_panel_frame_set_property;
  object_class->dispose      = mnb_panel_frame_dispose;
  object_class->finalize     = mnb_panel_frame_finalize;
  object_class->constructed  = mnb_panel_frame_constructed;

  actor_class->get_preferred_width  = mnb_panel_frame_get_preferred_width;
  actor_class->get_preferred_height = mnb_panel_frame_get_preferred_height;
  actor_class->allocate             = mnb_panel_frame_allocate;
  actor_class->paint                = mnb_panel_frame_paint;
  actor_class->pick                 = mnb_panel_frame_pick;
  actor_class->map                  = mnb_panel_frame_map;
  actor_class->unmap                = mnb_panel_frame_unmap;

  signals[FOOTER_CLICKED] =
    g_signal_new ("footer-clicked",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
mnb_panel_frame_init (MnbPanelFrame *self)
{
  self->priv = GET_PRIVATE (self);
}

MnbPanelFrame*
mnb_panel_frame_new (void)
{
  return g_object_new (MNB_TYPE_PANEL_FRAME, NULL);
}

