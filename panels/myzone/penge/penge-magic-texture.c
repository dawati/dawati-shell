/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <cogl/cogl.h>

#include "penge-magic-texture.h"

G_DEFINE_TYPE (PengeMagicTexture, penge_magic_texture, CLUTTER_TYPE_TEXTURE)

static void
penge_magic_texture_paint (ClutterActor *actor)
{
  ClutterActorBox box;
  CoglHandle *material, *tex;
  float bw, bh;
  float aw, ah;
  float v;
  float tx1, tx2, ty1, ty2;
  guint8 alpha;

  clutter_actor_get_allocation_box (actor, &box);
  material = clutter_texture_get_cogl_material (CLUTTER_TEXTURE (actor));
  tex = clutter_texture_get_cogl_texture (CLUTTER_TEXTURE (actor));

  bw = (float) cogl_texture_get_width (tex); /* base texture width */
  bh = (float) cogl_texture_get_height (tex); /* base texture height */

  aw = (float) (box.x2 - box.x1); /* allocation width */
  ah = (float) (box.y2 - box.y1); /* allocation height */

  /* no comment */
  if ((float)bw/bh < (float)aw/ah)
  {
    /* fit width */
    v = (((float)ah * bw) / ((float)aw * bh)) / 2;
    tx1 = 0;
    tx2 = 1;
    ty1 = (0.5 - v);
    ty2 = (0.5 + v);
  } else {
    /* fit height */
    v = (((float)aw * bh) / ((float)ah * bw)) / 2;
    tx1 = (0.5 - v);
    tx2 = (0.5 + v);
    ty1 = 0;
    ty2 = 1;
  }

  alpha = clutter_actor_get_paint_opacity (actor);

  cogl_material_set_color4ub (material,
                              alpha,
                              alpha,
                              alpha,
                              alpha);

  cogl_set_source (material);
  cogl_rectangle_with_texture_coords (0, 0,
                                      aw, ah,
                                      tx1, ty1,
                                      tx2, ty2);
}

static void
penge_magic_texture_class_init (PengeMagicTextureClass *klass)
{
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  actor_class->paint = penge_magic_texture_paint;
}

static void
penge_magic_texture_init (PengeMagicTexture *self)
{
}

