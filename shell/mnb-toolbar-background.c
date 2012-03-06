/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2012 Intel Corp.
 *
 * Authors: Lionel Landwerlin <lionel.g.landwerlin@linux.intel.com>
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

#include "mnb-toolbar-background.h"

G_DEFINE_TYPE (MnbToolbarBackground, mnb_toolbar_background, MX_TYPE_BOX_LAYOUT)

#define TOOLBAR_BACKGROUND_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_TOOLBAR_BACKGROUND, MnbToolbarBackgroundPrivate))

enum
{
  PROP_0,
  PROP_TOOLBAR
};

struct _MnbToolbarBackgroundPrivate
{
  MnbToolbar     *toolbar;
  ClutterTexture *parent_texture;

  gfloat          top;
  gfloat          right;
  gfloat          bottom;
  gfloat          left;
};

#if TOOLBAR_CUT_OUT
static void
mnb_toolbar_background_paint_background (MxWidget           *self,
                                         ClutterActor       *background,
                                         const ClutterColor *color)
{
  MnbToolbarBackgroundPrivate *priv = TOOLBAR_BACKGROUND_PRIVATE (self);
  ClutterActor *actor = (ClutterActor *) self;
  CoglHandle cogl_texture, cogl_material;
  ClutterActorBox box = { 0, };
  gfloat width, height;
  gfloat tex_width, tex_height;
  gfloat ex, ey;
  gfloat tx1, ty1, tx2, ty2;
  guint8 opacity;

  /* Copied from MxWidget:
   *
   * Default implementation just draws the background
   * colour and the image on top
   */
  if (color && color->alpha != 0)
    {
      ClutterColor bg_color = *color;

      bg_color.alpha = clutter_actor_get_paint_opacity (actor)
                       * bg_color.alpha
                       / 255;

      clutter_actor_get_allocation_box (actor, &box);

      width = box.x2 - box.x1;
      height = box.y2 - box.y1;

      cogl_set_source_color4ub (bg_color.red,
                                bg_color.green,
                                bg_color.blue,
                                bg_color.alpha);
      cogl_rectangle (0, 0, width, height);
    }

  /*
   * Copied from MxTextureFrame
   */

  /* no need to paint stuff if we don't have a texture */
  if (G_UNLIKELY (priv->parent_texture == NULL))
    return;

  cogl_texture = clutter_texture_get_cogl_texture (priv->parent_texture);
  if (cogl_texture == COGL_INVALID_HANDLE)
    return;
  cogl_material = clutter_texture_get_cogl_material (priv->parent_texture);
  if (cogl_material == COGL_INVALID_HANDLE)
    return;

  tex_width  = cogl_texture_get_width (cogl_texture);
  tex_height = cogl_texture_get_height (cogl_texture);

  clutter_actor_get_allocation_box (actor, &box);
  width = box.x2 - box.x1;
  height = box.y2 - box.y1;

  opacity = clutter_actor_get_paint_opacity (actor);

  /* Paint using the parent texture's material. It should already have
     the cogl texture set as the first layer */
  /* NB: for correct blending we need set a preumultiplied color here: */
  cogl_material_set_color4ub (cogl_material,
                              opacity, opacity, opacity, opacity);

  selector_texture = mnb_toolbar_get_selector_texture (priv->toolbar);

  cogl_material_set_layer (cogl_material, 1, selector_texture);
  cogl_material_set_layer_wrap_mode (cogl_material, 1,
                                     COGL_MATERIAL_WRAP_MODE_CLAMP_TO_EDGE);

  if (!cogl_material_set_layer_combine (cogl_material, 1,
                                        "RGBA = MODULATE(PREVIOUS,TEXTURE)",
                                        &error))
    {
      g_warning (G_STRLOC ": Error setting layer combine blend string: %s",
                 error->message);
      g_error_free (error);
    }

  cogl_set_source (cogl_material);

  /* simple stretch */
  if (priv->left == 0 && priv->right == 0 && priv->top == 0
      && priv->bottom == 0)
    {
      float spot_width, spot_height;
      float coords[8] = {
        0, 0, 1, 1,
        0, 0, 0, 0
      };

      mnb_toolbar_get_selector_allocation_box (priv->toolbar, &box);
      spot_width = box.x2 - box.x1;
      spot_height = box.y2 - box.y1;
      coords[4] = -(box.x1 / width) * (width / spot_width);
      coords[5] = -(box.y1 / height) * (height / spot_height);
      coords[6] = width / spot_width - (box.x1 / width) * (width / spot_width);
      coords[7] = height / spot_height - (box.y1 / height) * (height / spot_height);
      cogl_rectangle_with_multitexture_coords (0, 0, width, height, coords, 8);
      return;
    }

  tx1 = priv->left / tex_width;
  tx2 = (tex_width - priv->right) / tex_width;
  ty1 = priv->top / tex_height;
  ty2 = (tex_height - priv->bottom) / tex_height;

  ex = width - priv->right;
  if (ex < priv->left)
    ex = priv->left;

  ey = height - priv->bottom;
  if (ey < priv->top)
    ey = priv->top;

  {
    GLfloat rectangles[] =
    {
      /* top left corner */
      0, 0,
      priv->left, priv->top,
      0.0, 0.0,
      tx1, ty1,

      /* top middle */
      priv->left, 0,
      MAX (priv->left, ex), priv->top,
      tx1, 0.0,
      tx2, ty1,

      /* top right */
      ex, 0,
      MAX (ex + priv->right, width), priv->top,
      tx2, 0.0,
      1.0, ty1,

      /* mid left */
      0, priv->top,
      priv->left,  ey,
      0.0, ty1,
      tx1, ty2,

      /* center */
      priv->left, priv->top,
      ex, ey,
      tx1, ty1,
      tx2, ty2,

      /* mid right */
      ex, priv->top,
      MAX (ex + priv->right, width), ey,
      tx2, ty1,
      1.0, ty2,

      /* bottom left */
      0, ey,
      priv->left, MAX (ey + priv->bottom, height),
      0.0, ty2,
      tx1, 1.0,

      /* bottom center */
      priv->left, ey,
      ex, MAX (ey + priv->bottom, height),
      tx1, ty2,
      tx2, 1.0,

      /* bottom right */
      ex, ey,
      MAX (ex + priv->right, width), MAX (ey + priv->bottom, height),
      tx2, ty2,
      1.0, 1.0
    };

    cogl_rectangles_with_texture_coords (rectangles, 9);
  }
}
#endif /* TOOLBAR_CUT_OUT */

static void
mnb_toolbar_background_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  MnbToolbarBackgroundPrivate *priv = MNB_TOOLBAR_BACKGROUND (object)->priv;

  switch (property_id)
    {
    case PROP_TOOLBAR:
      g_value_set_object (value, priv->toolbar);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_toolbar_background_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  MnbToolbarBackgroundPrivate *priv = MNB_TOOLBAR_BACKGROUND (object)->priv;

  switch (property_id)
    {
    case PROP_TOOLBAR:
      priv->toolbar = (MnbToolbar *) g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_toolbar_background_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnb_toolbar_background_parent_class)->dispose (object);
}

static void
mnb_toolbar_background_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_toolbar_background_parent_class)->finalize (object);
}

static void
mnb_toolbar_background_class_init (MnbToolbarBackgroundClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
#if TOOLBAR_CUT_OUT
  MxWidgetClass *widget_class = MX_WIDGET_CLASS (klass);
#endif /* TOOLBAR_CUT_OUT */

  g_type_class_add_private (klass, sizeof (MnbToolbarBackgroundPrivate));

  object_class->get_property = mnb_toolbar_background_get_property;
  object_class->set_property = mnb_toolbar_background_set_property;
  object_class->dispose = mnb_toolbar_background_dispose;
  object_class->finalize = mnb_toolbar_background_finalize;

#if TOOLBAR_CUT_OUT
  widget_class->paint_background = mnb_toolbar_background_paint_background;
#endif /* TOOLBAR_CUT_OUT */

  g_object_class_install_property (object_class,
                                   PROP_TOOLBAR,
                                   g_param_spec_object ("toolbar",
                                                        "Toolbar",
                                                        "Toolbar",
                                                        MNB_TYPE_TOOLBAR,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

#if TOOLBAR_CUT_OUT
static void
mnb_toolbar_background_border_changed (MnbToolbarBackground *self,
                                       GParamSpec           *param,
                                       gpointer              data)
{
  MxTextureFrame *texture;
  MnbToolbarBackgroundPrivate *priv = self->priv;

  texture = (MxTextureFrame *) mx_widget_get_border_image (MX_WIDGET (self));
  if (!texture)
    return;

  priv->parent_texture =
    mx_texture_frame_get_parent_texture (texture);
  mx_texture_frame_get_border_values (texture,
                                      &priv->top,
                                      &priv->right,
                                      &priv->bottom,
                                      &priv->left);
}
#endif /* TOOLBAR_CUT_OUT */

static void
mnb_toolbar_background_init (MnbToolbarBackground *self)
{
  self->priv = TOOLBAR_BACKGROUND_PRIVATE (self);

#if TOOLBAR_CUT_OUT
  g_signal_connect (self, "notify",
                    G_CALLBACK (mnb_toolbar_background_border_changed),
                    NULL);
#endif /* TOOLBAR_CUT_OUT */
}

ClutterActor *
mnb_toolbar_background_new (MnbToolbar *toolbar)
{
  return g_object_new (MNB_TYPE_TOOLBAR_BACKGROUND,
                       "toolbar", toolbar,
                       NULL);
}
