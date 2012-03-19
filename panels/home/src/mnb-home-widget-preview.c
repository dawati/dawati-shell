/*
 * Copyright (c) 2012 Intel Corp.
 *
 * Author: Danielle Madeley <danielle.madeley@collabora.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "mnb-home-widget-preview.h"
#include "utils.h"

G_DEFINE_TYPE (MnbHomeWidgetPreview, mnb_home_widget_preview, MX_TYPE_TABLE);

enum /* properties */
{
  PROP_0,
  PROP_MODULE,
  PROP_ICON,
  PROP_LABEL,
  PROP_LAST
};

struct _MnbHomeWidgetPreviewPrivate
{
  char *module;
  ClutterActor *image;
  ClutterActor *label;
};

static void
mnb_home_widget_preview_get_property (GObject *self,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  MnbHomeWidgetPreviewPrivate *priv = MNB_HOME_WIDGET_PREVIEW (self)->priv;

  switch (prop_id)
    {
      case PROP_MODULE:
        g_value_set_string (value, priv->module);
        break;

      case PROP_ICON:
        g_object_get_property (G_OBJECT (priv->image),
            "filename", value);
        break;

      case PROP_LABEL:
        g_object_get_property (G_OBJECT (priv->label),
            "text", value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
        break;
    }
}

static void
mnb_home_widget_preview_set_property (GObject *self,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  MnbHomeWidgetPreviewPrivate *priv = MNB_HOME_WIDGET_PREVIEW (self)->priv;

  switch (prop_id)
    {
      case PROP_MODULE:
        g_free (priv->module);
        priv->module = g_value_dup_string (value);
        break;

      case PROP_ICON:
        g_object_set_property (G_OBJECT (priv->image),
            "filename", value);
        break;

      case PROP_LABEL:
        g_object_set_property (G_OBJECT (priv->label),
            "text", value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
        break;
    }
}

static void
mnb_home_widget_preview_finalize (GObject *self)
{
  MnbHomeWidgetPreviewPrivate *priv = MNB_HOME_WIDGET_PREVIEW (self)->priv;

  g_free (priv->module);

  G_OBJECT_CLASS (mnb_home_widget_preview_parent_class)->finalize (self);
}

static void
mnb_home_widget_preview_class_init (MnbHomeWidgetPreviewClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = mnb_home_widget_preview_get_property;
  gobject_class->set_property = mnb_home_widget_preview_set_property;
  gobject_class->finalize = mnb_home_widget_preview_finalize;

  g_type_class_add_private (gobject_class,
      sizeof (MnbHomeWidgetPreviewPrivate));

  g_object_class_install_property (gobject_class, PROP_MODULE,
      g_param_spec_string ("module",
        "Module",
        "",
        NULL,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_ICON,
      g_param_spec_string ("icon",
        "Icon",
        "",
        NULL,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LABEL,
      g_param_spec_string ("label",
        "Label",
        "",
        NULL,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
mnb_home_widget_preview_init (MnbHomeWidgetPreview *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      MNB_TYPE_HOME_WIDGET_PREVIEW, MnbHomeWidgetPreviewPrivate);

  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);

  self->priv->image = clutter_texture_new ();
  mx_table_add_actor (MX_TABLE (self), self->priv->image, 0, 0);
  clutter_actor_show (self->priv->image);

  self->priv->label = mx_label_new ();
  mx_table_add_actor (MX_TABLE (self), self->priv->label, 1, 0);
  clutter_actor_show (self->priv->label);
}

const char *
mnb_home_widget_preview_get_module (MnbHomeWidgetPreview *self)
{
  g_return_val_if_fail (MNB_IS_HOME_WIDGET_PREVIEW (self), NULL);

  return self->priv->module;
}
