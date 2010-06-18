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
mpl_panel_background_allocate (ClutterActor          *actor,
                               const ClutterActorBox *box,
                               ClutterAllocationFlags flags)
{
  MplPanelBackgroundPrivate *priv = MPL_PANEL_BACKGROUND (actor)->priv;
  ClutterActor              *border;

  border = mx_widget_get_border_image (MX_WIDGET (actor));

  /*
   * We need to use different background asset based on the size allocated to us
   * -- basically, the panel cannot be smaller than the size of the borders of
   * its border image (see MB#702), so if it is smaller than the current asset
   * allows, we change the asset to simpler one.
   *
   * The code here assumes that we either have no name, or are called
   * 'too-small', but since this is a private widget of libmeego-panel, we can
   * enforce this assumption.
   */
  if (border)
    {
      const gchar *name = clutter_actor_get_name (actor);
      gboolean     too_small = FALSE;

      /*
       * Get the dimensions of the base texture (we are guaranteed to be called
       * first with name == NULL, so this works).
       */
      if (!priv->base_geom_known && !name)
        {
          mx_texture_frame_get_border_values (MX_TEXTURE_FRAME (border),
                                              &priv->base_t,
                                              &priv->base_r,
                                              &priv->base_b,
                                              &priv->base_l);

          priv->base_geom_known = TRUE;
        }

      if (priv->base_l + priv->base_r > box->x2 - box->x1 ||
          priv->base_t + priv->base_b > box->y2 - box->y1)
        {
          too_small = TRUE;
        }

      if (!name && too_small)
        {
          clutter_actor_set_name (actor, "too-small");
        }
      else if (name && !too_small)
        {
          clutter_actor_set_name (actor, NULL);
        }
    }

  CLUTTER_ACTOR_CLASS (
             mpl_panel_background_parent_class)->allocate (actor, box, flags);
}

/*
 * Paints the provided texture frame trimming to the area indicated by padding.
 *
 * (Basically, we have one asset that gets split into two parts: an border area
 * that matches the padding, and the inside; MplPanelBackground paints the
 * latter, while the border is painted by the compositor as the window shadow.)
 */
static void
mpl_panel_background_paint_border_image (MxTextureFrame *frame,
                                         MxPadding      *padding)
{
  CoglHandle cogl_texture = COGL_INVALID_HANDLE;
  CoglHandle cogl_material = COGL_INVALID_HANDLE;
  ClutterActorBox box = { 0, };
  gfloat width, height;
  gfloat tex_width, tex_height;
  gfloat ex, ey;
  gfloat tx1, ty1, tx2, ty2;
  gfloat left, right, top, bottom;
  guint8 opacity;
  ClutterTexture *parent_texture;
  gfloat margin_l, margin_r, margin_t, margin_b;

  parent_texture = mx_texture_frame_get_parent_texture (frame);

  mx_texture_frame_get_border_values (frame, &top, &right, &bottom, &left);

  /* no need to paint stuff if we don't have a texture */
  if (G_UNLIKELY (parent_texture == NULL))
    return;

  /* parent texture may have been hidden, so need to make sure it gets
   * realized
   */
  if (!CLUTTER_ACTOR_IS_REALIZED (parent_texture))
    clutter_actor_realize (CLUTTER_ACTOR (parent_texture));

  cogl_texture = clutter_texture_get_cogl_texture (parent_texture);

  if (cogl_texture == COGL_INVALID_HANDLE)
    return;

  cogl_material = clutter_texture_get_cogl_material (parent_texture);

  if (cogl_material == COGL_INVALID_HANDLE)
    return;

  tex_width  = cogl_texture_get_width (cogl_texture);
  tex_height = cogl_texture_get_height (cogl_texture);

  clutter_actor_get_allocation_box ((ClutterActor*) frame, &box);

  width  = box.x2 - box.x1;
  height = box.y2 - box.y1;

  /*
   * These are the margins we are to trim expressed in texture coordinates.
   */
  margin_l = padding->left / tex_width;
  margin_r = padding->right / tex_width;
  margin_t = padding->top / tex_height;
  margin_b = padding->bottom / tex_height;

  tx1 = left / tex_width;
  tx2 = (tex_width - right) / tex_width;
  ty1 = top / tex_height;
  ty2 = (tex_height - bottom) / tex_height;

  ex = width - right;
  if (ex < 0)
    ex = right;

  ey = height - bottom;
  if (ey < 0)
    ey = bottom;

  opacity = clutter_actor_get_paint_opacity ((ClutterActor*)frame);

  cogl_material_set_color4ub (cogl_material,
                              opacity, opacity, opacity, opacity);
  cogl_set_source (cogl_material);

  cogl_material_set_layer_filters (cogl_material,
                                   0,
                                   COGL_MATERIAL_FILTER_NEAREST,
                                   COGL_MATERIAL_FILTER_NEAREST);

  {
    GLfloat rectangles[] =
    {
      /* top left corner */
      0.0, 0.0, left, top,
      margin_l, margin_t,
      tx1, ty1,

      /* top middle */
      left, 0.0, ex, top,
      tx1, margin_t,
      tx2, ty1,

      /* top right */
      ex, 0.0, width, top,
      tx2, margin_t,
      1.0 - margin_r, ty1,

      /* mid left */
      0.0, top, left, ey,
      margin_l, ty1,
      tx1, ty2,

      /* center */
      left, top, ex, ey,
      tx1, ty1,
      tx2, ty2,

      /* mid right */
      ex, top, width, ey,
      tx2, ty1,
      1.0 - margin_r, ty2,

      /* bottom left */
      0.0, ey, left, height,
      margin_l, ty2,
      tx1, 1.0 - margin_b,

      /* bottom center */
      left, ey, ex, height,
      tx1, ty2,
      tx2, 1.0 - margin_b,

      /* bottom right */
      ex, ey, width, height,
      tx2, ty2,
      1.0 - margin_r, 1.0 - margin_b
    };

    cogl_rectangles_with_texture_coords (rectangles, 9);
  }
}

/*
 * MxWidget::paint_background vfunction implementation.
 */
static void
mpl_panel_background_paint_background (MxWidget           *self,
                                       ClutterActor       *background,
                                       const ClutterColor *color)
{
  MxPadding padding = { 0, };

  mx_widget_get_padding (self, &padding);

  /*
   * Paint any solid background colour that is not completely transparent.
   */
  if (color && color->alpha != 0)
    {
      ClutterActor *actor = CLUTTER_ACTOR (self);
      ClutterActorBox allocation = { 0, };
      ClutterColor bg_color = *color;
      gfloat w, h;

      bg_color.alpha = clutter_actor_get_paint_opacity (actor)
                       * bg_color.alpha
                       / 255;

      clutter_actor_get_allocation_box (actor, &allocation);

      w = allocation.x2 - allocation.x1;
      h = allocation.y2 - allocation.y1;

      cogl_set_source_color4ub (bg_color.red,
                                bg_color.green,
                                bg_color.blue,
                                bg_color.alpha);
      cogl_rectangle (padding.left, padding.top,
                      w - padding.left - padding.right,
                      h - padding.top - padding.bottom);
    }

  /*
   * Now paint the asset.
   */
  if (background)
    mpl_panel_background_paint_border_image (MX_TEXTURE_FRAME (background),
                                             &padding);
}

static void
mpl_panel_background_class_init (MplPanelBackgroundClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);
  MxWidgetClass     *widget_class = MX_WIDGET_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MplPanelBackgroundPrivate));

  object_class->get_property        = mpl_panel_background_get_property;
  object_class->set_property        = mpl_panel_background_set_property;
  object_class->dispose             = mpl_panel_background_dispose;
  object_class->finalize            = mpl_panel_background_finalize;

  actor_class->get_preferred_width  = mpl_panel_background_get_preferred_width;
  actor_class->get_preferred_height = mpl_panel_background_get_preferred_height;
  actor_class->allocate             = mpl_panel_background_allocate;

  widget_class->paint_background    = mpl_panel_background_paint_background;
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

