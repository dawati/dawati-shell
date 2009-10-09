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


#ifndef _DALSTON_BRIGHTNESS_MANAGER
#define _DALSTON_BRIGHTNESS_MANAGER

#include <glib-object.h>

G_BEGIN_DECLS

#define DALSTON_TYPE_BRIGHTNESS_MANAGER dalston_brightness_manager_get_type()

#define DALSTON_BRIGHTNESS_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DALSTON_TYPE_BRIGHTNESS_MANAGER, DalstonBrightnessManager))

#define DALSTON_BRIGHTNESS_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DALSTON_TYPE_BRIGHTNESS_MANAGER, DalstonBrightnessManagerClass))

#define DALSTON_IS_BRIGHTNESS_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DALSTON_TYPE_BRIGHTNESS_MANAGER))

#define DALSTON_IS_BRIGHTNESS_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DALSTON_TYPE_BRIGHTNESS_MANAGER))

#define DALSTON_BRIGHTNESS_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DALSTON_TYPE_BRIGHTNESS_MANAGER, DalstonBrightnessManagerClass))

typedef struct {
  GObject parent;
} DalstonBrightnessManager;

typedef struct {
  GObjectClass parent_class;
  void (*num_levels_changed) (DalstonBrightnessManager *manager, gint num_levels);
  void (*brightness_changed) (DalstonBrightnessManager *manager, gint new_value);
} DalstonBrightnessManagerClass;

GType dalston_brightness_manager_get_type (void);

DalstonBrightnessManager *dalston_brightness_manager_new (void);

void dalston_brightness_manager_start_monitoring (DalstonBrightnessManager *manager);
void dalston_brightness_manager_stop_monitoring (DalstonBrightnessManager *manager);
void dalston_brightness_manager_set_brightness (DalstonBrightnessManager *manager,
                                                gint                      value);
gboolean dalston_brightness_manager_is_controllable (DalstonBrightnessManager *manager);

DalstonBrightnessManager *dalston_brightness_manager_dup_singleton (void);

void dalston_brightness_manager_maximise (DalstonBrightnessManager *manager);
void dalston_brightness_manager_increase (DalstonBrightnessManager *manager);
void dalston_brightness_manager_decrease (DalstonBrightnessManager *manager);

G_END_DECLS

#endif /* _DALSTON_BRIGHTNESS_MANAGER */

