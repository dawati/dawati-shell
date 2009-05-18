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


#ifndef _DALSTON_VOLUME_SLIDER
#define _DALSTON_VOLUME_SLIDER

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DALSTON_TYPE_VOLUME_SLIDER dalston_volume_slider_get_type()

#define DALSTON_VOLUME_SLIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DALSTON_TYPE_VOLUME_SLIDER, DalstonVolumeSlider))

#define DALSTON_VOLUME_SLIDER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DALSTON_TYPE_VOLUME_SLIDER, DalstonVolumeSliderClass))

#define DALSTON_IS_VOLUME_SLIDER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DALSTON_TYPE_VOLUME_SLIDER))

#define DALSTON_IS_VOLUME_SLIDER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DALSTON_TYPE_VOLUME_SLIDER))

#define DALSTON_VOLUME_SLIDER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DALSTON_TYPE_VOLUME_SLIDER, DalstonVolumeSliderClass))

typedef struct {
  GtkHScale parent;
} DalstonVolumeSlider;

typedef struct {
  GtkHScaleClass parent_class;
} DalstonVolumeSliderClass;

GType dalston_volume_slider_get_type (void);

GtkWidget *dalston_volume_slider_new (void);

G_END_DECLS

#endif /* _DALSTON_VOLUME_SLIDER */

