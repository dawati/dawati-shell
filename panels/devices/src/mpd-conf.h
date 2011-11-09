
/*
 * Copyright © 2011 Intel Corp.
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

#ifndef MPD_CONF_H
#define MPD_CONF_H

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MPD_TYPE_CONF mpd_conf_get_type()

#define MPD_CONF(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_CONF, MpdConf))

#define MPD_CONF_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_CONF, MpdConfClass))

#define MPD_IS_CONF(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_CONF))

#define MPD_IS_CONF_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_CONF))

#define MPD_CONF_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_CONF, MpdConfClass))

typedef struct
{
  GObject parent;
} MpdConf;

typedef struct
{
  GObjectClass parent;
} MpdConfClass;

GType
mpd_conf_get_type (void);

MpdConf *
mpd_conf_new (void);

bool
mpd_conf_get_brightness_enabled (MpdConf *self);

float
mpd_conf_get_brightness_value (MpdConf *self);

void
mpd_conf_set_brightness_value (MpdConf *self,
                               float    value);

float
mpd_conf_get_brightness_value_battery (MpdConf *self);

void
mpd_conf_set_brightness_value_battery (MpdConf *self,
                                       float    value);

int
mpd_conf_get_suspend_idle_time (MpdConf *self);

typedef enum
{
  MPD_CONF_LID_ACTION_NONE,     /* Must be first. */
  MPD_CONF_LID_ACTION_SUSPEND   /* Must be last. */
} MpdConfLidAction;

MpdConfLidAction
mpd_conf_get_lid_action (MpdConf *self);

G_END_DECLS

#endif /* MPD_CONF_H */

