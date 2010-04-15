
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
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

#ifndef MPD_IDLE_MANAGER_H
#define MPD_IDLE_MANAGER_H

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MPD_TYPE_IDLE_MANAGER mpd_idle_manager_get_type()

#define MPD_IDLE_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_IDLE_MANAGER, MpdIdleManager))

#define MPD_IDLE_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_IDLE_MANAGER, MpdIdleManagerClass))

#define MPD_IS_IDLE_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_IDLE_MANAGER))

#define MPD_IS_IDLE_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_IDLE_MANAGER))

#define MPD_IDLE_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_IDLE_MANAGER, MpdIdleManagerClass))

typedef struct
{
  GObject parent;
} MpdIdleManager;

typedef struct
{
  GObjectClass parent;
} MpdIdleManagerClass;

GType
mpd_idle_manager_get_type (void);

MpdIdleManager *
mpd_idle_manager_new (void);

bool
mpd_idle_manager_activate_screensaver (MpdIdleManager   *self,
                                       GError          **error);

bool
mpd_idle_manager_suspend (MpdIdleManager   *self,
                          GError          **error);

G_END_DECLS

#endif /* MPD_IDLE_MANAGER_H */

