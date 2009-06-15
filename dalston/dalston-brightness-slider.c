/*
 * Dalston - power and volume applets for Moblin
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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
 *
 */


#include "dalston-brightness-slider.h"
#include <dalston/dalston-brightness-manager.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (DalstonBrightnessSlider, dalston_brightness_slider, GTK_TYPE_HSCALE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_BRIGHTNESS_SLIDER, DalstonBrightnessSliderPrivate))

typedef struct _DalstonBrightnessSliderPrivate DalstonBrightnessSliderPrivate;

struct _DalstonBrightnessSliderPrivate {
  DalstonBrightnessManager *manager;

  gint num_levels;
  gint cur_value;
};

enum
{
  PROP_0,
  PROP_MANAGER
};

static void
dalston_brightness_slider_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  DalstonBrightnessSliderPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_MANAGER:
      g_value_set_object (value, priv->manager);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_brightness_slider_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  DalstonBrightnessSliderPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_MANAGER:
      priv->manager = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_brightness_slider_dispose (GObject *object)
{
  DalstonBrightnessSliderPrivate *priv = GET_PRIVATE (object);

  if (priv->manager)
  {
    g_object_unref (priv->manager);
    priv->manager = NULL;
  }

  G_OBJECT_CLASS (dalston_brightness_slider_parent_class)->dispose (object);
}

static void
dalston_brightness_slider_finalize (GObject *object)
{
  G_OBJECT_CLASS (dalston_brightness_slider_parent_class)->finalize (object);
}

static void
_range_value_changed_cb (GtkRange *range,
                         gpointer  userdata)
{
  DalstonBrightnessSliderPrivate *priv = GET_PRIVATE (userdata);
  gint value;

  value = (gint)gtk_range_get_value (range);
  gtk_range_set_fill_level (GTK_RANGE (range),
                            (gdouble)value);

  dalston_brightness_manager_set_brightness (priv->manager,
                                             value);
}

static void
dalston_brightness_slider_update (DalstonBrightnessSlider *slider)
{
  DalstonBrightnessSliderPrivate *priv = GET_PRIVATE (slider);

  if (priv->num_levels > 0)
  {
    g_signal_handlers_block_by_func (slider,
                                     _range_value_changed_cb, slider);
    gtk_range_set_value (GTK_RANGE (slider), priv->cur_value);
    gtk_range_set_fill_level (GTK_RANGE (slider),
                            (gdouble)priv->cur_value);
    g_signal_handlers_unblock_by_func (slider,
                                       _range_value_changed_cb, 
                                       slider);
  }
}

static void
_manager_brightness_changed_cb (DalstonBrightnessManager *manager,
                                gint                      value,
                                gpointer                  userdata)
{
  DalstonBrightnessSlider *slider = (DalstonBrightnessSlider *)userdata;
  DalstonBrightnessSliderPrivate *priv = GET_PRIVATE (userdata);

  priv->cur_value = value;
  dalston_brightness_slider_update (slider);
}

static void
_manager_num_levels_changed_cb (DalstonBrightnessManager *manager,
                                gint                      num_levels,
                                gpointer                  userdata)
{
  DalstonBrightnessSlider *slider = (DalstonBrightnessSlider *)userdata;
  DalstonBrightnessSliderPrivate *priv = GET_PRIVATE (userdata);

  priv->num_levels = num_levels;
  gtk_range_set_range (GTK_RANGE (slider), 0, num_levels - 1);
  gtk_range_set_increments (GTK_RANGE (slider),
                            1,
                            1);
  dalston_brightness_slider_update (slider);
}

static void
dalston_brightness_slider_constructed (GObject *object)
{
  DalstonBrightnessSlider *slider = (DalstonBrightnessSlider *)object;
  DalstonBrightnessSliderPrivate *priv = GET_PRIVATE (object);

  g_signal_connect (priv->manager,
                    "num-levels-changed",
                    (GCallback)_manager_num_levels_changed_cb,
                    object);

  g_signal_connect (priv->manager,
                    "brightness-changed",
                    (GCallback)_manager_brightness_changed_cb,
                    object);

  gtk_scale_set_digits (GTK_SCALE (slider),
                        0);

  gtk_range_set_restrict_to_fill_level (GTK_RANGE (slider),
                                        FALSE);
  gtk_range_set_show_fill_level (GTK_RANGE (slider),
                                 TRUE);

  if (G_OBJECT_CLASS (dalston_brightness_slider_parent_class)->constructed)
  {
    G_OBJECT_CLASS (dalston_brightness_slider_parent_class)->constructed (object);
  }
}

static void
dalston_brightness_slider_class_init (DalstonBrightnessSliderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (DalstonBrightnessSliderPrivate));

  object_class->get_property = dalston_brightness_slider_get_property;
  object_class->set_property = dalston_brightness_slider_set_property;
  object_class->dispose = dalston_brightness_slider_dispose;
  object_class->finalize = dalston_brightness_slider_finalize;
  object_class->constructed = dalston_brightness_slider_constructed;

  pspec = g_param_spec_object ("manager",
                               "Brightness manager",
                               "The brightness manager that we're going "
                               "to read from / control",
                               DALSTON_TYPE_BRIGHTNESS_MANAGER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_MANAGER, pspec);
}

static gchar *
_scale_format_value_cb (GtkScale *scale,
                        gdouble   value,
                        gpointer  userdata)
{
  DalstonBrightnessSliderPrivate *priv = GET_PRIVATE (userdata);
  gdouble percentage;
  gchar *format = NULL;

  percentage = (double)value/(double)(priv->num_levels - 1) * 100.0;

  if (percentage == 100.0)
    format = g_strdup (_("Brighter than the sun"));
  else if (percentage >= 90.0)
    format = g_strdup (_("Very bright"));
  else if (percentage >= 75.0)
    format = g_strdup (_("Bright"));
  else if (percentage > 50.0)
    format = g_strdup (_("Morning has broken"));
  else if (percentage == 50.0)
    format = g_strdup (_("Midday"));
  else if (percentage >= 25.0)
    format = g_strdup (_("Pretty dusky"));
  else if (percentage >= 10.0)
    format = g_strdup (_("Twilight"));
  else if (percentage > 0.0)
    format = g_strdup (_("Very dark"));
  else
    format = g_strdup (_("Total eclipse"));

  return format;
}

static void
dalston_brightness_slider_init (DalstonBrightnessSlider *self)
{
  gtk_scale_set_digits (GTK_SCALE (self), 0);

  g_signal_connect (self,
                    "value-changed",
                    (GCallback)_range_value_changed_cb,
                    self);
  gtk_scale_set_value_pos (GTK_SCALE (self), GTK_POS_BOTTOM);

  g_signal_connect (self,
                    "format-value",
                    (GCallback)_scale_format_value_cb,
                    self);

}

GtkWidget *
dalston_brightness_slider_new (DalstonBrightnessManager *manager)
{
  return g_object_new (DALSTON_TYPE_BRIGHTNESS_SLIDER, 
                       "manager",
                       manager,
                       NULL);
}


