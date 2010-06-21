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

#ifndef _CARRICK_APPLET
#define _CARRICK_APPLET

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CARRICK_TYPE_APPLET carrick_applet_get_type ()

#define CARRICK_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CARRICK_TYPE_APPLET, CarrickApplet))

#define CARRICK_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CARRICK_TYPE_APPLET, CarrickAppletClass))

#define CARRICK_IS_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CARRICK_TYPE_APPLET))

#define CARRICK_IS_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CARRICK_TYPE_APPLET))

#define CARRICK_APPLET_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CARRICK_TYPE_APPLET, CarrickAppletClass))

typedef struct _CarrickApplet CarrickApplet;
typedef struct _CarrickAppletClass CarrickAppletClass;
typedef struct _CarrickAppletPrivate CarrickAppletPrivate;

struct _CarrickApplet
{
  GObject parent;
  CarrickAppletPrivate *priv;
};

struct _CarrickAppletClass
{
  GObjectClass parent_class;
};

GType carrick_applet_get_type (void);

CarrickApplet* carrick_applet_new (void);
GtkWidget* carrick_applet_get_pane (CarrickApplet *applet);

G_END_DECLS

#endif /* _CARRICK_APPLET */
