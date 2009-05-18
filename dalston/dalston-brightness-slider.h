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


#ifndef _DALSTON_BRIGHTNESS_SLIDER
#define _DALSTON_BRIGHTNESS_SLIDER

#include <glib-object.h>
#include <gtk/gtk.h>

#include <dalston/dalston-brightness-manager.h>

G_BEGIN_DECLS

#define DALSTON_TYPE_BRIGHTNESS_SLIDER dalston_brightness_slider_get_type()

#define DALSTON_BRIGHTNESS_SLIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DALSTON_TYPE_BRIGHTNESS_SLIDER, DalstonBrightnessSlider))

#define DALSTON_BRIGHTNESS_SLIDER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DALSTON_TYPE_BRIGHTNESS_SLIDER, DalstonBrightnessSliderClass))

#define DALSTON_IS_BRIGHTNESS_SLIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DALSTON_TYPE_BRIGHTNESS_SLIDER))

#define DALSTON_IS_BRIGHTNESS_SLIDER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DALSTON_TYPE_BRIGHTNESS_SLIDER))

#define DALSTON_BRIGHTNESS_SLIDER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DALSTON_TYPE_BRIGHTNESS_SLIDER, DalstonBrightnessSliderClass))

typedef struct {
  GtkHScale parent;
} DalstonBrightnessSlider;

typedef struct {
  GtkHScaleClass parent_class;
} DalstonBrightnessSliderClass;

GType dalston_brightness_slider_get_type (void);

GtkWidget *dalston_brightness_slider_new (DalstonBrightnessManager *manager);

G_END_DECLS

#endif /* _DALSTON_BRIGHTNESS_SLIDER */

