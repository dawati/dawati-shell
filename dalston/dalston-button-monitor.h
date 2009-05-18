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


#ifndef _DALSTON_BUTTON_MONITOR
#define _DALSTON_BUTTON_MONITOR

#include <glib-object.h>

G_BEGIN_DECLS

#define DALSTON_TYPE_BUTTON_MONITOR dalston_button_monitor_get_type()

#define DALSTON_BUTTON_MONITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DALSTON_TYPE_BUTTON_MONITOR, DalstonButtonMonitor))

#define DALSTON_BUTTON_MONITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DALSTON_TYPE_BUTTON_MONITOR, DalstonButtonMonitorClass))

#define DALSTON_IS_BUTTON_MONITOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DALSTON_TYPE_BUTTON_MONITOR))

#define DALSTON_IS_BUTTON_MONITOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DALSTON_TYPE_BUTTON_MONITOR))

#define DALSTON_BUTTON_MONITOR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DALSTON_TYPE_BUTTON_MONITOR, DalstonButtonMonitorClass))

typedef struct {
  GObject parent;
} DalstonButtonMonitor;

typedef struct {
  GObjectClass parent_class;
} DalstonButtonMonitorClass;

GType dalston_button_monitor_get_type (void);

DalstonButtonMonitor *dalston_button_monitor_new (void);

G_END_DECLS

#endif /* _DALSTON_BUTTON_MONITOR */

