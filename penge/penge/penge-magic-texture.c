#include <cogl/cogl.h>

#include "penge-magic-texture.h"

G_DEFINE_TYPE (PengeMagicTexture, penge_magic_texture, CLUTTER_TYPE_TEXTURE)

static void
penge_magic_texture_paint (ClutterActor *actor)
{
  ClutterActorBox box;
  CoglHandle *tex;
  gint bw, bh;
  gint aw, ah;
  float v;
  ClutterFixed tx1, tx2, ty1, ty2;
  ClutterColor col = { 0xff, 0xff, 0xff, 0xff };

  clutter_actor_get_allocation_box (actor, &box);
  tex = clutter_texture_get_cogl_texture (CLUTTER_TEXTURE (actor));

  bw = cogl_texture_get_width (tex); /* base texture width */
  bh = cogl_texture_get_height (tex); /* base texture height */

  aw = CLUTTER_UNITS_TO_INT (box.x2 - box.x1); /* allocation width */
  ah = CLUTTER_UNITS_TO_INT (box.y2 - box.y1); /* allocation height */

  /* no comment */
  if ((float)bw/bh < (float)aw/ah)
  {
    /* fit width */
    v = (((float)ah * bw) / ((float)aw * bh)) / 2;
    tx1 = 0;
    tx2 = CLUTTER_INT_TO_FIXED (1);
    ty1 = CLUTTER_FLOAT_TO_FIXED (0.5 - v);
    ty2 = CLUTTER_FLOAT_TO_FIXED (0.5 + v);
  } else {
    /* fit height */
    v = (((float)aw * bh) / ((float)ah * bw)) / 2;
    tx1 = CLUTTER_FLOAT_TO_FIXED (0.5 - v);
    tx2 = CLUTTER_FLOAT_TO_FIXED (0.5 + v);
    ty1 = 0;
    ty2 = CLUTTER_INT_TO_FIXED (1);
  }

  col.alpha = clutter_actor_get_paint_opacity (actor);
  cogl_set_source_color4ub (col.red, col.green, col.blue, col.alpha);
  cogl_rectangle (0, 0, aw, ah);
  cogl_set_source_texture (tex);
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

