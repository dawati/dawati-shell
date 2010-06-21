/*
 * Carrick - a connection panel for the MeeGo Netbook
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

#ifndef _CARRICK_PANE
#define _CARRICK_PANE

#include <gtk/gtk.h>
#include "carrick-icon-factory.h"
#include "carrick-notification-manager.h"

G_BEGIN_DECLS

#define CARRICK_TYPE_PANE carrick_pane_get_type ()

#define CARRICK_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CARRICK_TYPE_PANE, CarrickPane))

#define CARRICK_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CARRICK_TYPE_PANE, CarrickPaneClass))

#define CARRICK_IS_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CARRICK_TYPE_PANE))

#define CARRICK_IS_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CARRICK_TYPE_PANE))

#define CARRICK_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CARRICK_TYPE_PANE, CarrickPaneClass))

typedef struct _CarrickPane CarrickPane;
typedef struct _CarrickPaneClass CarrickPaneClass;
typedef struct _CarrickPanePrivate CarrickPanePrivate;

struct _CarrickPane
{
  GtkHBox parent;

  CarrickPanePrivate *priv;
};

struct _CarrickPaneClass
{
  GtkHBoxClass parent_class;

  void (*connection_changed) (CarrickPane   *self,
                              const gchar   *connection_type,
                              const gchar   *connection_name,
                              const gchar   *connection_state,
                              guint          strength);
};

GType carrick_pane_get_type (void);

GtkWidget* carrick_pane_new (CarrickIconFactory         *icon_factory,
                             CarrickNotificationManager *notifications);

void carrick_pane_update (CarrickPane *pane);

G_END_DECLS

#endif /* _CARRICK_PANE */
