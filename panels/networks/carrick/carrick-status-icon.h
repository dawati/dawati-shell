/*
 * Carrick - a connection panel for the Moblin Netbook
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Written by - Joshua Lock <josh@linux.intel.com>
 *
 */

#ifndef _CARRICK_STATUS_ICON
#define _CARRICK_STATUS_ICON

#include <gtk/gtk.h>
#include <gconnman/gconnman.h>
#include "carrick-icon-factory.h"

G_BEGIN_DECLS

#define CARRICK_TYPE_STATUS_ICON carrick_status_icon_get_type()

#define CARRICK_STATUS_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CARRICK_TYPE_STATUS_ICON, CarrickStatusIcon))

#define CARRICK_STATUS_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CARRICK_TYPE_STATUS_ICON, CarrickStatusIconClass))

#define CARRICK_IS_STATUS_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CARRICK_TYPE_STATUS_ICON))

#define CARRICK_IS_STATUS_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CARRICK_TYPE_STATUS_ICON))

#define CARRICK_STATUS_ICON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CARRICK_TYPE_STATUS_ICON, CarrickStatusIconClass))

typedef struct {
  GtkStatusIcon parent;
} CarrickStatusIcon;

typedef struct {
  GtkStatusIconClass parent_class;
} CarrickStatusIconClass;

GType carrick_status_icon_get_type (void);

GtkWidget* carrick_status_icon_new (CarrickIconFactory *factory,
                                    CmManager          *manager);

void carrick_status_icon_set_active (CarrickStatusIcon *icon,
                                     gboolean           active);
void carrick_status_icon_update_manager (CarrickStatusIcon *icon,
                                         CmManager         *manager);
void carrick_status_icon_update (CarrickStatusIcon *icon);
void carrick_status_icon_set_connecting (CarrickStatusIcon *icon);
G_END_DECLS

#endif /* _CARRICK_STATUS_ICON */
