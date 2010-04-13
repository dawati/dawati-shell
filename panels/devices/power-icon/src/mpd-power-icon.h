
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

#ifndef MPD_POWER_ICON_H
#define MPD_POWER_ICON_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MPD_TYPE_POWER_ICON mpd_power_icon_get_type()

#define MPD_POWER_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPD_TYPE_POWER_ICON, MpdPowerIcon))

#define MPD_POWER_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPD_TYPE_POWER_ICON, MpdPowerIconClass))

#define MPD_IS_POWER_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPD_TYPE_POWER_ICON))

#define MPD_IS_POWER_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPD_TYPE_POWER_ICON))

#define MPD_POWER_ICON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPD_TYPE_POWER_ICON, MpdPowerIconClass))

typedef struct
{
  GObject parent;
} MpdPowerIcon;

typedef struct
{
  GObjectClass parent;
} MpdPowerIconClass;

GType
mpd_power_icon_get_type (void);

MpdPowerIcon *
mpd_power_icon_new (void);

void
mpd_power_icon_test_notification (MpdPowerIcon *self,
                                  unsigned int  percentage);

G_END_DECLS

#endif /* MPD_POWER_ICON_H */

