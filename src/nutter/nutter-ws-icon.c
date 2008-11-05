/*
 * Authored By Tomas Frydrych <tf@linux.intel.com>
 *
 * Copyright (C) 2008 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <clutter/clutter-actor.h>
#include <clutter/clutter-rectangle.h>
#include <clutter/clutter-label.h>
#include <string.h>

#include "nutter-ws-icon.h"

typedef struct _NutterWsIconActorData NutterWsIconActorData;

static void nutter_ws_icon_dispose             (GObject *object);
static void nutter_ws_icon_finalize            (GObject *object);
static void nutter_ws_icon_set_property        (GObject      *object,
						guint         prop_id,
						const GValue *value,
						GParamSpec   *pspec);
static void nutter_ws_icon_get_property        (GObject      *object,
						guint         prop_id,
						GValue       *value,
						GParamSpec   *pspec);

static void nutter_ws_icon_paint (ClutterActor *actor);

static void nutter_ws_icon_pick (ClutterActor *actor,
				 const ClutterColor *color);

static void
nutter_ws_icon_get_preferred_width (ClutterActor *self,
				    ClutterUnit for_height,
				    ClutterUnit *min_width_p,
				    ClutterUnit *natural_width_p);

static void
nutter_ws_icon_get_preferred_height (ClutterActor *self,
				     ClutterUnit for_width,
				     ClutterUnit *min_height_p,
				     ClutterUnit *natural_height_p);

static void nutter_ws_icon_allocate (ClutterActor *self,
				  const ClutterActorBox *box,
				  gboolean absolute_origin_changed);

G_DEFINE_TYPE (NutterWsIcon, nutter_ws_icon, CLUTTER_TYPE_ACTOR);

#define NUTTER_WS_ICON_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NUTTER_TYPE_WS_ICON, \
                                NutterWsIconPrivate))

struct _NutterWsIconPrivate
{
  ClutterActor *rectangle;
  ClutterActor *label;
};

enum
{
  PROP_0,
  PROP_COLOR,
  PROP_BORDER_COLOR,
  PROP_BORDER_WIDTH,
  PROP_TEXT_COLOR,
  PROP_FONT_NAME,
  PROP_TEXT,
};

static void
nutter_ws_icon_class_init (NutterWsIconClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  ClutterActorClass *actor_class = (ClutterActorClass *) klass;

  gobject_class->dispose = nutter_ws_icon_dispose;
  gobject_class->finalize = nutter_ws_icon_finalize;

  gobject_class->set_property = nutter_ws_icon_set_property;
  gobject_class->get_property = nutter_ws_icon_get_property;

  actor_class->paint                = nutter_ws_icon_paint;
  actor_class->get_preferred_width  = nutter_ws_icon_get_preferred_width;
  actor_class->get_preferred_height = nutter_ws_icon_get_preferred_height;
  actor_class->allocate             = nutter_ws_icon_allocate;

  g_type_class_add_private (klass, sizeof (NutterWsIconPrivate));


  g_object_class_install_property (gobject_class,
                                   PROP_BORDER_WIDTH,
                                   g_param_spec_uint ("border-width",
                                                      "Border Width",
                                                      "The width of the border of the rectangle",
                                                      0, G_MAXUINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_COLOR,
                                   g_param_spec_boxed ("color",
                                                       "Color",
                                                       "The color of the rectangle",
                                                       CLUTTER_TYPE_COLOR,
                                                       G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_TEXT_COLOR,
                                   g_param_spec_boxed ("text-color",
                                                       "Text Color",
                                                       "The color of the text",
                                                       CLUTTER_TYPE_COLOR,
                                                       G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_BORDER_COLOR,
                                   g_param_spec_boxed ("border-color",
                                                       "Border Color",
                                                       "The color of the border of the rectangle",
                                                       CLUTTER_TYPE_COLOR,
                                                       G_PARAM_READWRITE));


  g_object_class_install_property (gobject_class,
				   PROP_FONT_NAME,
				   g_param_spec_string ("font-name",
							"Font Name",
							"Pango font description",
							NULL,
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
				   PROP_TEXT,
				   g_param_spec_string ("text",
							"Text",
							"Text to render",
							NULL,
							G_PARAM_READWRITE));
}

static void
nutter_ws_icon_init (NutterWsIcon *self)
{
  NutterWsIconPrivate *priv;
  ClutterRectangle    *rectangle;
  ClutterLabel        *label;
  ClutterColor black = { 0, 0, 0, 0xff };
  ClutterColor white = { 0xff, 0xff, 0xff, 0xff };

  self->priv = priv = NUTTER_WS_ICON_GET_PRIVATE (self);

  priv->rectangle = clutter_rectangle_new_with_color (&black);
  clutter_actor_set_parent (priv->rectangle, CLUTTER_ACTOR (self));
  rectangle = CLUTTER_RECTANGLE (priv->rectangle);

  priv->label = clutter_label_new ();
  clutter_actor_set_parent (priv->label, CLUTTER_ACTOR (self));
  label = CLUTTER_LABEL (priv->label);

  clutter_rectangle_set_border_width (rectangle, 3);
  clutter_rectangle_set_border_color (rectangle, &white);

  clutter_label_set_font_name (label, "Sans 12");
  clutter_label_set_color (label, &white);
}

static void
nutter_ws_icon_dispose (GObject *object)
{
  G_OBJECT_CLASS (nutter_ws_icon_parent_class)->dispose (object);
}

static void
nutter_ws_icon_finalize (GObject *object)
{
  G_OBJECT_CLASS (nutter_ws_icon_parent_class)->finalize (object);
}

static void
nutter_ws_icon_set_property (GObject      *object,
			  guint         prop_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
  NutterWsIcon        *icon = NUTTER_WS_ICON (object);
  NutterWsIconPrivate *priv = icon->priv;

  if (!priv->rectangle || !priv->label)
    return;

  switch (prop_id)
    {
    case PROP_TEXT:
      clutter_label_set_text (CLUTTER_LABEL (priv->label),
			      g_value_get_string (value));
      break;
    case PROP_FONT_NAME:
      clutter_label_set_font_name (CLUTTER_LABEL (priv->label),
				   g_value_get_string (value));
      break;
    case PROP_BORDER_COLOR:
      clutter_rectangle_set_border_color (CLUTTER_RECTANGLE (priv->rectangle),
                                          g_value_get_boxed (value));
      break;
    case PROP_BORDER_WIDTH:
      clutter_rectangle_set_border_width (CLUTTER_RECTANGLE (priv->rectangle),
					  clutter_value_get_unit (value));
      break;
    case PROP_COLOR:
      clutter_rectangle_set_color (CLUTTER_RECTANGLE (priv->rectangle),
				   g_value_get_boxed (value));
      break;
    case PROP_TEXT_COLOR:
      clutter_label_set_color (CLUTTER_LABEL (priv->label),
				                    g_value_get_boxed (value));
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
nutter_ws_icon_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
  NutterWsIcon        *icon = NUTTER_WS_ICON (object);
  NutterWsIconPrivate *priv = icon->priv;
  ClutterColor         color;

  if (!priv->rectangle || !priv->label)
    return;

  switch (prop_id)
    {
    case PROP_TEXT:
      g_value_set_string (value,
			  clutter_label_get_text (CLUTTER_LABEL (priv->label)));
      break;
    case PROP_FONT_NAME:
      g_value_set_string (value,
		   clutter_label_get_font_name (CLUTTER_LABEL (priv->label)));
      break;
    case PROP_COLOR:
      clutter_rectangle_get_color (CLUTTER_RECTANGLE (priv->rectangle), &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_TEXT_COLOR:
      clutter_label_get_color (CLUTTER_LABEL (priv->label), &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_BORDER_COLOR:
      clutter_rectangle_get_border_color (CLUTTER_RECTANGLE (priv->rectangle),
					  &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_BORDER_WIDTH:
      g_value_set_uint (value,
			clutter_rectangle_get_border_width (
					CLUTTER_RECTANGLE (priv->rectangle)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

ClutterActor *
nutter_ws_icon_new (void)
{
  ClutterActor *self = g_object_new (NUTTER_TYPE_WS_ICON, NULL);

  return self;
}

static void
nutter_ws_icon_paint (ClutterActor *actor)
{
  NutterWsIcon *icon = (NutterWsIcon *) actor;
  NutterWsIconPrivate *priv = icon->priv;

  clutter_actor_paint (priv->rectangle);
  clutter_actor_paint (priv->label);
}

static void
nutter_ws_icon_get_preferred_width (ClutterActor *self,
				    ClutterUnit   for_height,
				    ClutterUnit  *min_width_p,
				    ClutterUnit  *natural_width_p)
{
  ClutterUnit natural_width = CLUTTER_UNITS_FROM_INT (100);

  if (min_width_p)
    *min_width_p = natural_width;
  if (natural_width_p)
    *natural_width_p = natural_width;
}

static void
nutter_ws_icon_get_preferred_height (ClutterActor *self,
				     ClutterUnit   for_width,
				     ClutterUnit  *min_height_p,
				     ClutterUnit  *natural_height_p)
{
  ClutterUnit natural_height = CLUTTER_UNITS_FROM_INT (80);

  if (min_height_p)
    *min_height_p = natural_height;
  if (natural_height_p)
    *natural_height_p = natural_height;
}

static void
nutter_ws_icon_allocate (ClutterActor          *self,
			 const ClutterActorBox *box,
			 gboolean               absolute_origin_changed)
{
  NutterWsIcon        *icon = (NutterWsIcon *) self;
  NutterWsIconPrivate *priv = icon->priv;
  gint                 border_width;
  ClutterActorBox      child_box;
  ClutterUnit          label_p_height;
  ClutterUnit          label_p_width;
  ClutterUnit          label_n_width;
  ClutterUnit          label_n_height;
  ClutterUnit          label_max_width;
  ClutterUnit          box_width;
  ClutterUnit          box_height;

  CLUTTER_ACTOR_CLASS (nutter_ws_icon_parent_class)
    ->allocate (self, box, absolute_origin_changed);

  border_width =
    clutter_rectangle_get_border_width (CLUTTER_RECTANGLE (priv->rectangle));

  box_width  = box->x2 - box->x1;
  box_height =  box->y2 - box->y1;

  child_box.x1 = 0;
  child_box.y1 = 0;
  child_box.x2 = box_width;
  child_box.y2 = box_height;

  clutter_actor_allocate (priv->rectangle, &child_box, absolute_origin_changed);

  label_max_width = box_width - CLUTTER_UNITS_FROM_INT (2 * border_width - 6);

  clutter_actor_get_preferred_height (priv->label, label_max_width, NULL,
				      &label_n_height);

  clutter_actor_get_preferred_width (priv->label, label_n_height, NULL,
				     &label_n_width);

  child_box.x1 = (box_width - label_n_width) / 2;
  child_box.y1 = (box_height - label_n_height) / 2;
  child_box.x2 = child_box.x1 + label_n_width;
  child_box.y2 = child_box.y1 + label_n_height;

  clutter_actor_allocate (priv->label, &child_box, absolute_origin_changed);
}

void
nutter_ws_icon_get_color (NutterWsIcon *self, ClutterColor *color)
{
  NutterWsIcon        *icon = (NutterWsIcon *) self;
  NutterWsIconPrivate *priv   = icon->priv;

  if (!priv->rectangle)
    return;

  clutter_rectangle_get_color (CLUTTER_RECTANGLE (priv->rectangle), color);
}

void
nutter_ws_icon_set_color (NutterWsIcon *self, const ClutterColor *color)
{
  NutterWsIcon        *icon = (NutterWsIcon *) self;
  NutterWsIconPrivate *priv   = icon->priv;

  if (!priv->rectangle)
    return;

  clutter_rectangle_set_color (CLUTTER_RECTANGLE (priv->rectangle), color);
}

guint
nutter_ws_icon_get_border_width (NutterWsIcon *self)
{
  NutterWsIcon        *icon = (NutterWsIcon *) self;
  NutterWsIconPrivate *priv   = icon->priv;

  if (!priv->rectangle)
    return 0;

  return clutter_rectangle_get_border_width(CLUTTER_RECTANGLE(priv->rectangle));
}

void
nutter_ws_icon_set_border_width (NutterWsIcon *self, guint width)
{
  NutterWsIcon        *icon = (NutterWsIcon *) self;
  NutterWsIconPrivate *priv   = icon->priv;

  if (!priv->rectangle)
    return;

  clutter_rectangle_set_border_width (CLUTTER_RECTANGLE (priv->rectangle),
				      width);
}

void
nutter_ws_icon_get_border_color (NutterWsIcon *self, ClutterColor *color)
{
  NutterWsIcon        *icon = (NutterWsIcon *) self;
  NutterWsIconPrivate *priv   = icon->priv;

  if (!priv->rectangle)
    return;

  clutter_rectangle_get_border_color (CLUTTER_RECTANGLE (priv->rectangle),
				      color);
}

void
nutter_ws_icon_set_border_color (NutterWsIcon       *self,
				 const ClutterColor *color)
{
  NutterWsIcon        *icon = (NutterWsIcon *) self;
  NutterWsIconPrivate *priv = icon->priv;

  if (!priv->rectangle)
    return;

  clutter_rectangle_set_border_color (CLUTTER_RECTANGLE (priv->rectangle),
				      color);
}

void
nutter_ws_icon_get_text_color (NutterWsIcon *self, ClutterColor *color)
{
  NutterWsIcon        *icon = (NutterWsIcon *) self;
  NutterWsIconPrivate *priv = icon->priv;

  if (!priv->label)
    return;

  clutter_label_get_color (CLUTTER_LABEL (priv->label), color);
}

void
nutter_ws_icon_set_text_color (NutterWsIcon       *self,
			       const ClutterColor *color)
{
  NutterWsIcon        *icon = (NutterWsIcon *) self;
  NutterWsIconPrivate *priv = icon->priv;

  if (!priv->label)
    return;

  clutter_label_set_color (CLUTTER_LABEL (priv->label), color);
}

const gchar *
nutter_ws_icon_get_font_name (NutterWsIcon *self)
{
  NutterWsIcon        *icon = (NutterWsIcon *) self;
  NutterWsIconPrivate *priv = icon->priv;

  if (!priv->label)
    return NULL;

  return clutter_label_get_font_name (CLUTTER_LABEL (priv->label));
}

void
nutter_ws_icon_set_font_name (NutterWsIcon *self, const gchar *font)
{
  NutterWsIcon        *icon = (NutterWsIcon *) self;
  NutterWsIconPrivate *priv = icon->priv;

  if (!priv->label)
    return;

  clutter_label_set_font_name (CLUTTER_LABEL (priv->label), font);
}

const gchar *
nutter_ws_icon_get_text (NutterWsIcon *self)
{
  NutterWsIcon        *icon = (NutterWsIcon *) self;
  NutterWsIconPrivate *priv = icon->priv;

  if (!priv->label)
    return NULL;

  return clutter_label_get_text (CLUTTER_LABEL (priv->label));
}

void
nutter_ws_icon_set_text (NutterWsIcon *self, const gchar *text)
{
  NutterWsIcon        *icon = (NutterWsIcon *) self;
  NutterWsIconPrivate *priv = icon->priv;

  if (!priv->label)
    return;

  clutter_label_set_text (CLUTTER_LABEL (priv->label), text);
}

