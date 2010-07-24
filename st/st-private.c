/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
#include <math.h>
#include <string.h>

#include "st-private.h"

/**
 * _st_actor_get_preferred_width:
 * @actor: a #ClutterActor
 * @for_height: as with clutter_actor_get_preferred_width()
 * @y_fill: %TRUE if @actor will fill its allocation vertically
 * @min_width_p: as with clutter_actor_get_preferred_width()
 * @natural_width_p: as with clutter_actor_get_preferred_width()
 *
 * Like clutter_actor_get_preferred_width(), but if @y_fill is %FALSE,
 * then it will compute a width request based on the assumption that
 * @actor will be given an allocation no taller than its natural
 * height.
 */
void
_st_actor_get_preferred_width  (ClutterActor *actor,
                                gfloat        for_height,
                                gboolean      y_fill,
                                gfloat       *min_width_p,
                                gfloat       *natural_width_p)
{
  if (!y_fill && for_height != -1)
    {
      ClutterRequestMode mode;
      gfloat natural_height;

      g_object_get (G_OBJECT (actor), "request-mode", &mode, NULL);
      if (mode == CLUTTER_REQUEST_WIDTH_FOR_HEIGHT)
        {
          clutter_actor_get_preferred_height (actor, -1, NULL, &natural_height);
          if (for_height > natural_height)
            for_height = natural_height;
        }
    }

  clutter_actor_get_preferred_width (actor, for_height, min_width_p, natural_width_p);
}

/**
 * _st_actor_get_preferred_height:
 * @actor: a #ClutterActor
 * @for_width: as with clutter_actor_get_preferred_height()
 * @x_fill: %TRUE if @actor will fill its allocation horizontally
 * @min_height_p: as with clutter_actor_get_preferred_height()
 * @natural_height_p: as with clutter_actor_get_preferred_height()
 *
 * Like clutter_actor_get_preferred_height(), but if @x_fill is
 * %FALSE, then it will compute a height request based on the
 * assumption that @actor will be given an allocation no wider than
 * its natural width.
 */
void
_st_actor_get_preferred_height (ClutterActor *actor,
                                gfloat        for_width,
                                gboolean      x_fill,
                                gfloat       *min_height_p,
                                gfloat       *natural_height_p)
{
  if (!x_fill && for_width != -1)
    {
      ClutterRequestMode mode;
      gfloat natural_width;

      g_object_get (G_OBJECT (actor), "request-mode", &mode, NULL);
      if (mode == CLUTTER_REQUEST_HEIGHT_FOR_WIDTH)
        {
          clutter_actor_get_preferred_width (actor, -1, NULL, &natural_width);
          if (for_width > natural_width)
            for_width = natural_width;
        }
    }

  clutter_actor_get_preferred_height (actor, for_width, min_height_p, natural_height_p);
}

/**
 * _st_allocate_fill:
 * @parent: the parent #StWidget
 * @child: the child (not necessarily an #StWidget)
 * @childbox: total space that could be allocated to @child
 * @x_alignment: horizontal alignment within @childbox
 * @y_alignment: vertical alignment within @childbox
 * @x_fill: whether or not to fill @childbox horizontally
 * @y_fill: whether or not to fill @childbox vertically
 *
 * Given @childbox, containing the initial allocation of @child, this
 * adjusts the horizontal allocation if @x_fill is %FALSE, and the
 * vertical allocation if @y_fill is %FALSE, by:
 *
 *     - reducing the allocation if it is larger than @child's natural
 *       size.
 *
 *     - adjusting the position of the child within the allocation
 *       according to @x_alignment/@y_alignment (and flipping
 *       @x_alignment if @parent has %ST_TEXT_DIRECTION_RTL)
 *
 * If @x_fill and @y_fill are both %TRUE, or if @child's natural size
 * is larger than the initial allocation in @childbox, then @childbox
 * will be unchanged.
 *
 * If you are allocating children with _st_allocate_fill(), you should
 * determine their preferred sizes using
 * _st_actor_get_preferred_width() and
 * _st_actor_get_preferred_height(), not with the corresponding
 * Clutter methods.
 */
void
_st_allocate_fill (StWidget        *parent,
                   ClutterActor    *child,
                   ClutterActorBox *childbox,
                   StAlign          x_alignment,
                   StAlign          y_alignment,
                   gboolean         x_fill,
                   gboolean         y_fill)
{
  gfloat natural_width, natural_height;
  gfloat min_width, min_height;
  gfloat child_width, child_height;
  gfloat available_width, available_height;
  ClutterRequestMode request;
  gdouble x_align, y_align;

  available_width  = childbox->x2 - childbox->x1;
  available_height = childbox->y2 - childbox->y1;

  if (available_width < 0)
    {
      available_width = 0;
      childbox->x2 = childbox->x1;
    }

  if (available_height < 0)
    {
      available_height = 0;
      childbox->y2 = childbox->y1;
    }

  /* If we are filling both horizontally and vertically then we don't
   * need to do anything else.
   */
  if (x_fill && y_fill)
    return;

  _st_get_align_factors (parent, x_alignment, y_alignment,
                         &x_align, &y_align);

  /* The following is based on clutter_actor_get_preferred_size(), but
   * modified to cope with the fact that the available size may be
   * less than the preferred size.
   */
  request = CLUTTER_REQUEST_HEIGHT_FOR_WIDTH;
  g_object_get (G_OBJECT (child), "request-mode", &request, NULL);

  if (request == CLUTTER_REQUEST_HEIGHT_FOR_WIDTH)
    {
      clutter_actor_get_preferred_width (child, -1,
                                         &min_width,
                                         &natural_width);

      child_width = CLAMP (natural_width, min_width, available_width);

      clutter_actor_get_preferred_height (child, child_width,
                                          &min_height,
                                          &natural_height);

      child_height = CLAMP (natural_height, min_height, available_height);
    }
  else
    {
      clutter_actor_get_preferred_height (child, -1,
                                          &min_height,
                                          &natural_height);

      child_height = CLAMP (natural_height, min_height, available_height);

      clutter_actor_get_preferred_width (child, child_height,
                                         &min_width,
                                         &natural_width);

      child_width = CLAMP (natural_width, min_width, available_width);
    }

  if (!x_fill)
    {
      childbox->x1 += (int)((available_width - child_width) * x_align);
      childbox->x2 = childbox->x1 + (int) child_width;
    }

  if (!y_fill)
    {
      childbox->y1 += (int)((available_height - child_height) * y_align);
      childbox->y2 = childbox->y1 + (int) child_height;
    }
}

/**
 * _st_get_align_factors:
 * @widget: an #StWidget
 * @x_align: an #StAlign
 * @y_align: an #StAlign
 * @x_align_out: (out) (allow-none): @x_align as a #gdouble
 * @y_align_out: (out) (allow-none): @y_align as a #gdouble
 *
 * Converts @x_align and @y_align to #gdouble values. If @widget has
 * %ST_TEXT_DIRECTION_RTL, the @x_align_out value will be flipped
 * relative to @x_align.
 */
void
_st_get_align_factors (StWidget *widget,
                       StAlign   x_align,
                       StAlign   y_align,
                       gdouble  *x_align_out,
                       gdouble  *y_align_out)
{
  if (x_align_out)
    {
      switch (x_align)
        {
        case ST_ALIGN_START:
          *x_align_out = 0.0;
          break;

        case ST_ALIGN_MIDDLE:
          *x_align_out = 0.5;
          break;

        case ST_ALIGN_END:
          *x_align_out = 1.0;
          break;

        default:
          g_warn_if_reached ();
          break;
        }

      if (st_widget_get_direction (widget) == ST_TEXT_DIRECTION_RTL)
        *x_align_out = 1.0 - *x_align_out;
    }

  if (y_align_out)
    {
      switch (y_align)
        {
        case ST_ALIGN_START:
          *y_align_out = 0.0;
          break;

        case ST_ALIGN_MIDDLE:
          *y_align_out = 0.5;
          break;

        case ST_ALIGN_END:
          *y_align_out = 1.0;
          break;

        default:
          g_warn_if_reached ();
          break;
        }
    }
}

/**
 * _st_set_text_from_style:
 * @text: Target #ClutterText
 * @theme_node: Source #StThemeNode
 *
 * Set various GObject properties of the @text object using
 * CSS information from @theme_node.
 */
void
_st_set_text_from_style (ClutterText *text,
                         StThemeNode *theme_node)
{

  ClutterColor color;
  StTextDecoration decoration;
  PangoAttrList *attribs;
  const PangoFontDescription *font;
  gchar *font_string;
  StTextAlign align;

  st_theme_node_get_foreground_color (theme_node, &color);
  clutter_text_set_color (text, &color);

  font = st_theme_node_get_font (theme_node);
  font_string = pango_font_description_to_string (font);
  clutter_text_set_font_name (text, font_string);
  g_free (font_string);

  attribs = pango_attr_list_new ();

  decoration = st_theme_node_get_text_decoration (theme_node);
  if (decoration & ST_TEXT_DECORATION_UNDERLINE)
    {
      PangoAttribute *underline = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
      pango_attr_list_insert (attribs, underline);
    }
  if (decoration & ST_TEXT_DECORATION_LINE_THROUGH)
    {
      PangoAttribute *strikethrough = pango_attr_strikethrough_new (TRUE);
      pango_attr_list_insert (attribs, strikethrough);
    }
  /* Pango doesn't have an equivalent attribute for _OVERLINE, and we deliberately
   * skip BLINK (for now...)
   */

  clutter_text_set_attributes (text, attribs);

  pango_attr_list_unref (attribs);

  align = st_theme_node_get_text_align (theme_node);
  if(align == ST_TEXT_ALIGN_JUSTIFY) {
    clutter_text_set_justify (text, TRUE);
    clutter_text_set_line_alignment (text, PANGO_ALIGN_LEFT);
  } else {
    clutter_text_set_justify (text, FALSE);
    clutter_text_set_line_alignment (text, (PangoAlignment) align);
  }
}


/*****
 * Shadows
 *****/

static gdouble *
calculate_gaussian_kernel (gdouble   sigma,
                           guint     n_values)
{
  gdouble *ret, sum;
  gdouble exp_divisor;
  gint half, i;

  g_return_val_if_fail (sigma > 0, NULL);

  half = n_values / 2;

  ret = g_malloc (n_values * sizeof (gdouble));
  sum = 0.0;

  exp_divisor = 2 * sigma * sigma;

  /* n_values of 1D Gauss function */
  for (i = 0; i < n_values; i++)
    {
      ret[i] = exp (-(i - half) * (i - half) / exp_divisor);
      sum += ret[i];
    }

  /* normalize */
  for (i = 0; i < n_values; i++)
    ret[i] /= sum;

  return ret;
}

CoglHandle
_st_create_shadow_material (StShadow   *shadow_spec,
                            CoglHandle  src_texture)
{
  CoglHandle  material;
  CoglHandle  texture;
  guchar     *pixels_in, *pixels_out;
  gint        width_in, height_in, rowstride_in;
  gint        width_out, height_out, rowstride_out;
  float       sigma;

  g_return_val_if_fail (shadow_spec != NULL, COGL_INVALID_HANDLE);
  g_return_val_if_fail (src_texture != COGL_INVALID_HANDLE,
                        COGL_INVALID_HANDLE);

  /* we use an approximation of the sigma - blur radius relationship used
     in Firefox for doing SVG blurs; see
     http://mxr.mozilla.org/mozilla-central/source/gfx/thebes/src/gfxBlur.cpp#280
  */
  sigma = shadow_spec->blur / 1.9;

  width_in  = cogl_texture_get_width  (src_texture);
  height_in = cogl_texture_get_height (src_texture);
  rowstride_in = (width_in + 3) & ~3;

  pixels_in  = g_malloc0 (rowstride_in * height_in);

  cogl_texture_get_data (src_texture, COGL_PIXEL_FORMAT_A_8,
                         rowstride_in, pixels_in);

  if ((guint) shadow_spec->blur == 0)
    {
      width_out  = width_in;
      height_out = height_in;
      rowstride_out = rowstride_in;
      pixels_out = g_memdup (pixels_in, rowstride_out * height_out);
    }
  else
    {
      gdouble *kernel;
      guchar  *line;
      gint     n_values, half;
      gint     x_in, y_in, x_out, y_out, i;

      n_values = (gint) 5 * sigma;
      half = n_values / 2;

      width_out  = width_in  + 2 * half;
      height_out = height_in + 2 * half;
      rowstride_out = (width_out + 3) & ~3;

      pixels_out = g_malloc0 (rowstride_out * height_out);
      line       = g_malloc0 (rowstride_out);

      kernel = calculate_gaussian_kernel (sigma, n_values);

      /* vertical blur */
      for (x_in = 0; x_in < width_in; x_in++)
        for (y_out = 0; y_out < height_out; y_out++)
          {
            guchar *pixel_in, *pixel_out;
            gint i0, i1;

            y_in = y_out - half;

            /* We read from the source at 'y = y_in + i - half'; clamp the
             * full i range [0, n_values) so that y is in [0, height_in).
             */
            i0 = MAX (half - y_in, 0);
            i1 = MIN (height_in + half - y_in, n_values);

            pixel_in  =  pixels_in + (y_in + i0 - half) * rowstride_in + x_in;
            pixel_out =  pixels_out + y_out * rowstride_out + (x_in + half);

            for (i = i0; i < i1; i++)
              {
                *pixel_out += *pixel_in * kernel[i];
                pixel_in += rowstride_in;
              }
          }

      /* horizontal blur */
      for (y_out = 0; y_out < height_out; y_out++)
        {
          memcpy (line, pixels_out + y_out * rowstride_out, rowstride_out);

          for (x_out = 0; x_out < width_out; x_out++)
            {
              gint i0, i1;
              guchar *pixel_out, *pixel_in;

              /* We read from the source at 'x = x_out + i - half'; clamp the
               * full i range [0, n_values) so that x is in [0, width_out).
               */
              i0 = MAX (half - x_out, 0);
              i1 = MIN (width_out + half - x_out, n_values);

              pixel_in  = line + x_out + i0 - half;
              pixel_out = pixels_out + rowstride_out * y_out + x_out;

              *pixel_out = 0;
              for (i = i0; i < i1; i++)
                {
                  *pixel_out += *pixel_in * kernel[i];
                  pixel_in++;
                }
            }
        }
      g_free (kernel);
      g_free (line);
    }

  texture = cogl_texture_new_from_data (width_out,
                                        height_out,
                                        COGL_TEXTURE_NONE,
                                        COGL_PIXEL_FORMAT_A_8,
                                        COGL_PIXEL_FORMAT_A_8,
                                        rowstride_out,
                                        pixels_out);

  g_free (pixels_in);
  g_free (pixels_out);

  material = cogl_material_new ();

  cogl_material_set_layer (material, 0, texture);

  /* We set up the material to blend the shadow texture with the combine
   * constant, but defer setting the latter until painting, so that we can
   * take the actor's overall opacity into account. */
  cogl_material_set_layer_combine (material, 0,
                                   "RGBA = MODULATE (CONSTANT, TEXTURE[A])",
                                   NULL);


  cogl_handle_unref (texture);

  return material;
}

CoglHandle
_st_create_shadow_material_from_actor (StShadow     *shadow_spec,
                                       ClutterActor *actor)
{
  CoglHandle shadow_material = COGL_INVALID_HANDLE;

  if (CLUTTER_IS_TEXTURE (actor))
    {
      CoglHandle texture;

      texture = clutter_texture_get_cogl_texture (CLUTTER_TEXTURE (actor));
      shadow_material = _st_create_shadow_material (shadow_spec, texture);
    }
  else
    {
      CoglHandle buffer, offscreen;
      ClutterActorBox box;
      float width, height;

      clutter_actor_get_allocation_box (actor, &box);
      clutter_actor_box_get_size (&box, &width, &height);

      buffer = cogl_texture_new_with_size (width,
                                           height,
                                           COGL_TEXTURE_NO_SLICING,
                                           COGL_PIXEL_FORMAT_ANY);
      offscreen = cogl_offscreen_new_to_texture (buffer);

      if (offscreen != COGL_INVALID_HANDLE)
        {
          CoglColor clear_color;

          cogl_color_set_from_4ub (&clear_color, 0, 0, 0, 0);
          cogl_push_framebuffer (offscreen);
          cogl_clear (&clear_color, COGL_BUFFER_BIT_COLOR);
          cogl_ortho (0, width, height, 0, 0, 1.0);
          clutter_actor_paint (actor);
          cogl_pop_framebuffer ();
          cogl_handle_unref (offscreen);

          shadow_material = _st_create_shadow_material (shadow_spec, buffer);
        }

      cogl_handle_unref (buffer);
    }

  return shadow_material;
}

void
_st_paint_shadow_with_opacity (StShadow        *shadow_spec,
                               CoglHandle       shadow_material,
                               ClutterActorBox *box,
                               guint8           paint_opacity)
{
  ClutterActorBox shadow_box;
  CoglColor       color;

  g_return_if_fail (shadow_spec != NULL);
  g_return_if_fail (shadow_material != COGL_INVALID_HANDLE);

  st_shadow_get_box (shadow_spec, box, &shadow_box);

  cogl_color_set_from_4ub (&color,
                           shadow_spec->color.red   * paint_opacity / 255,
                           shadow_spec->color.green * paint_opacity / 255,
                           shadow_spec->color.blue  * paint_opacity / 255,
                           shadow_spec->color.alpha * paint_opacity / 255);
  cogl_color_premultiply (&color);

  cogl_material_set_layer_combine_constant (shadow_material, 0, &color);

  cogl_set_source (shadow_material);
  cogl_rectangle_with_texture_coords (shadow_box.x1, shadow_box.y1,
                                      shadow_box.x2, shadow_box.y2,
                                      0, 0, 1, 1);
}
