/* mwb-separator.c */
/*
 * Copyright (c) 2009 Intel Corp.
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

/* Borrowed from the moblin-web-browser project */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter/clutter.h>

#include "mwb-separator.h"

static void nbtk_stylable_iface_init (NbtkStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MwbSeparator, mwb_separator, NBTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (NBTK_TYPE_STYLABLE,
                                                nbtk_stylable_iface_init));

#define MWB_SEPARATOR_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MWB_TYPE_SEPARATOR, \
                                MwbSeparatorPrivate))

struct _MwbSeparatorPrivate
{
  gfloat line_width;
  gfloat on_width, off_width;
  ClutterColor color;
};

enum
{
  PROP_0,

  PROP_COLOR,
  PROP_LINE_WIDTH,
  PROP_OFF_WIDTH,
  PROP_ON_WIDTH
};

static const ClutterColor default_color = { 0, 0, 0, 255 };

static void
mwb_separator_get_property (GObject *object, guint property_id,
                            GValue *value, GParamSpec *pspec)
{
  MwbSeparator *separator = MWB_SEPARATOR (object);

  switch (property_id)
    {
    case PROP_COLOR:
      {
        ClutterColor color;
        mwb_separator_get_color (separator, &color);
        clutter_value_set_color (value, &color);
      }
      break;

    case PROP_LINE_WIDTH:
      g_value_set_float (value, mwb_separator_get_line_width (separator));
      break;

    case PROP_OFF_WIDTH:
      g_value_set_float (value, mwb_separator_get_off_width (separator));
      break;

    case PROP_ON_WIDTH:
      g_value_set_float (value, mwb_separator_get_on_width (separator));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mwb_separator_set_property (GObject *object, guint property_id,
                            const GValue *value, GParamSpec *pspec)
{
  MwbSeparator *separator = MWB_SEPARATOR (object);

  switch (property_id)
    {
    case PROP_COLOR:
      mwb_separator_set_color (separator, clutter_value_get_color (value));
      break;

    case PROP_LINE_WIDTH:
      mwb_separator_set_line_width (separator, g_value_get_float (value));
      break;

    case PROP_OFF_WIDTH:
      mwb_separator_set_off_width (separator, g_value_get_float (value));
      break;

    case PROP_ON_WIDTH:
      mwb_separator_set_on_width (separator, g_value_get_float (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mwb_separator_paint (ClutterActor *actor)
{
  MwbSeparator *separator = MWB_SEPARATOR (actor);
  MwbSeparatorPrivate *priv = separator->priv;
  ClutterGeometry geom;
  NbtkPadding padding;
  guint8 tmp_alpha;
  gfloat ypos;

  /* compute the composited opacity of the actor taking into account
   * the opacity of the color set by the user
   */
  tmp_alpha = (clutter_actor_get_paint_opacity (actor)
               * priv->color.alpha
               / 255);

  cogl_set_source_color4ub (priv->color.red,
                            priv->color.green,
                            priv->color.blue,
                            tmp_alpha);

  clutter_actor_get_allocation_geometry (actor, &geom);

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  /* Center the line in the allocated height */
  ypos = ((geom.height - padding.top - padding.bottom) / 2.0f
          - priv->line_width / 2.0f
          + padding.top);

  /* If the widths don't progress forward then just draw a solid line
     instead */
  if (priv->on_width + priv->off_width <= 1e-8f)
    cogl_rectangle (padding.left,
                    ypos,
                    geom.width - padding.right,
                    ypos + priv->line_width);
  else
    {
      gfloat xpos = padding.left;

      gboolean on_off = TRUE;

      while (TRUE)
        {
          gfloat part_width = on_off ? priv->on_width : priv->off_width;

          if (xpos + part_width >= geom.width - padding.right)
            {
              /* Draw to the end and stop */
              if (on_off)
                cogl_rectangle (xpos, ypos, geom.width - padding.right,
                                ypos + priv->line_width);
              break;
            }

          if (on_off)
            cogl_rectangle (xpos, ypos,
                            xpos + part_width, ypos + priv->line_width);

          xpos += part_width;
          on_off = !on_off;
        }
    }
}

static void
mwb_separator_get_preferred_height (ClutterActor *self,
                                    gfloat        for_width,
                                    gfloat       *min_height_p,
                                    gfloat       *natural_height_p)
{
  MwbSeparator *separator = MWB_SEPARATOR (self);
  MwbSeparatorPrivate *priv = separator->priv;

  if (min_height_p)
    *min_height_p = 0;

  if (natural_height_p)
    {
      NbtkPadding padding;

      nbtk_widget_get_padding (NBTK_WIDGET (self), &padding);

      *natural_height_p = priv->line_width * 3.0f + padding.top +
                          padding.bottom;
    }
}

static void
mwb_separator_style_changed_cb (NbtkWidget *widget)
{
  MwbSeparator *self = MWB_SEPARATOR (widget);
  ClutterColor *color = NULL;
  double line_width;
  float /* line_width, */ on_width, off_width;

  nbtk_stylable_get (NBTK_STYLABLE (self),
                     "color", &color,
                     "line-width", &line_width,
                     "off-width", &off_width,
                     "on-width", &on_width,
                     NULL);

  if (color)
    {
      mwb_separator_set_color (self, color);
      clutter_color_free (color);
    }

  mwb_separator_set_line_width (self, line_width);
  mwb_separator_set_off_width (self, off_width);
  mwb_separator_set_on_width (self, on_width);
}

static void
nbtk_stylable_iface_init (NbtkStylableIface *iface)
{
  static gboolean is_initialized = FALSE;

  if (!is_initialized)
    {
      GParamSpec *pspec;

      is_initialized = TRUE;

      pspec = g_param_spec_double ("line-width",
                                  "Line width",
                                  "Width of the line in pixels",
                                  0.0, G_MAXFLOAT, 1.0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NAME |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
      nbtk_stylable_iface_install_property (iface, MWB_TYPE_SEPARATOR, pspec);

      pspec = g_param_spec_float ("off-width",
                                  "Off width",
                                  "Width of the off part of the dashes "
                                  "in pixels",
                                  0.0, G_MAXFLOAT, 0.0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NAME |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
      nbtk_stylable_iface_install_property (iface, MWB_TYPE_SEPARATOR, pspec);

      pspec = g_param_spec_float ("on-width",
                                  "On width",
                                  "Width of the on part of the dashes "
                                  "in pixels",
                                  0.0, G_MAXFLOAT, 0.0,
                                  G_PARAM_READWRITE |
                                  G_PARAM_STATIC_NAME |
                                  G_PARAM_STATIC_NICK |
                                  G_PARAM_STATIC_BLURB);
      nbtk_stylable_iface_install_property (iface, MWB_TYPE_SEPARATOR, pspec);
    }
}

static void
mwb_separator_class_init (MwbSeparatorClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  ClutterActorClass *actor_class = (ClutterActorClass *) klass;
  GParamSpec *pspec;

  object_class->get_property = mwb_separator_get_property;
  object_class->set_property = mwb_separator_set_property;
  actor_class->paint = mwb_separator_paint;
  actor_class->get_preferred_height = mwb_separator_get_preferred_height;

  g_type_class_add_private (klass, sizeof (MwbSeparatorPrivate));

  pspec = clutter_param_spec_color ("color",
                                    "Color",
                                    "The color of the line",
                                    &default_color,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NAME |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_COLOR, pspec);

  pspec = g_param_spec_float ("line-width",
                              "Line Width",
                              "The width of the line to draw",
                              0.0, G_MAXFLOAT, 1.0,
                              G_PARAM_READWRITE |
                              G_PARAM_STATIC_NAME |
                              G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_LINE_WIDTH, pspec);

  pspec = g_param_spec_float ("off-width",
                              "Off Width",
                              "The width of the 'off' part of the line",
                              0.0, G_MAXFLOAT, 0.0,
                              G_PARAM_READWRITE |
                              G_PARAM_STATIC_NAME |
                              G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_OFF_WIDTH, pspec);

  pspec = g_param_spec_float ("on-width",
                              "On Width",
                              "The width of the 'on' part of the line",
                              0.0, G_MAXFLOAT, 0.0,
                              G_PARAM_READWRITE |
                              G_PARAM_STATIC_NAME |
                              G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB);
  g_object_class_install_property (object_class, PROP_ON_WIDTH, pspec);
}

static void
mwb_separator_init (MwbSeparator *self)
{
  MwbSeparatorPrivate *priv;

  priv = self->priv = MWB_SEPARATOR_GET_PRIVATE (self);

  priv->color = default_color;
  priv->line_width = 1.0f;

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mwb_separator_style_changed_cb), NULL);
}

NbtkWidget *
mwb_separator_new (void)
{
  NbtkWidget *self = g_object_new (MWB_TYPE_SEPARATOR, NULL);

  return self;
}

void
mwb_separator_get_color (MwbSeparator *separator,
                         ClutterColor *color)
{
  MwbSeparatorPrivate *priv;

  g_return_if_fail (MWB_IS_SEPARATOR (separator));
  g_return_if_fail (color != NULL);

  priv = separator->priv;

  color->red = priv->color.red;
  color->green = priv->color.green;
  color->blue = priv->color.blue;
  color->alpha = priv->color.alpha;
}

void
mwb_separator_set_color (MwbSeparator       *separator,
                         const ClutterColor *color)
{
  MwbSeparatorPrivate *priv;

  g_return_if_fail (MWB_IS_SEPARATOR (separator));
  g_return_if_fail (color != NULL);

  g_object_ref (separator);

  priv = separator->priv;

  priv->color.red = color->red;
  priv->color.green = color->green;
  priv->color.blue = color->blue;
  priv->color.alpha = color->alpha;

  if (CLUTTER_ACTOR_IS_MAPPED (separator))
    clutter_actor_queue_redraw (CLUTTER_ACTOR (separator));

  g_object_notify (G_OBJECT (separator), "color");

  g_object_unref (separator);
}

#define MWB_SEPARATOR_FLOAT_ACCESSORS(name)                             \
  gfloat                                                                \
  mwb_separator_get_ ## name ## _width (MwbSeparator *separator)        \
  {                                                                     \
    g_return_val_if_fail (MWB_IS_SEPARATOR (separator), 0.0f);          \
    return separator->priv->name ## _width;                             \
  }                                                                     \
                                                                        \
  void                                                                  \
  mwb_separator_set_ ## name ## _width (MwbSeparator *separator,        \
                                        gfloat value)                   \
  {                                                                     \
    MwbSeparatorPrivate *priv;                                          \
                                                                        \
    g_return_if_fail (MWB_IS_SEPARATOR (separator));                    \
                                                                        \
    priv = separator->priv;                                             \
                                                                        \
    if (priv->name ## _width != value)                                  \
      {                                                                 \
        priv->name ## _width = value;                                   \
        if (CLUTTER_ACTOR_IS_MAPPED (CLUTTER_ACTOR (separator)))        \
          clutter_actor_queue_redraw (CLUTTER_ACTOR (separator));       \
        g_object_notify (G_OBJECT (separator),                          \
                         G_STRINGIFY (name) "-width");                  \
      }                                                                 \
  }

MWB_SEPARATOR_FLOAT_ACCESSORS (line)
MWB_SEPARATOR_FLOAT_ACCESSORS (off)
MWB_SEPARATOR_FLOAT_ACCESSORS (on)
