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


#ifndef _DALSTON_VOLUME_APPLET
#define _DALSTON_VOLUME_APPLET

#include <glib-object.h>
#include <gtk/gtk.h>
#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-panel-gtk.h>

G_BEGIN_DECLS

#define DALSTON_TYPE_VOLUME_APPLET dalston_volume_applet_get_type()

#define DALSTON_VOLUME_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DALSTON_TYPE_VOLUME_APPLET, DalstonVolumeApplet))

#define DALSTON_VOLUME_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), DALSTON_TYPE_VOLUME_APPLET, DalstonVolumeAppletClass))

#define DALSTON_IS_VOLUME_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DALSTON_TYPE_VOLUME_APPLET))

#define DALSTON_IS_VOLUME_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), DALSTON_TYPE_VOLUME_APPLET))

#define DALSTON_VOLUME_APPLET_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), DALSTON_TYPE_VOLUME_APPLET, DalstonVolumeAppletClass))

typedef struct {
  GObject parent;
} DalstonVolumeApplet;

typedef struct {
  GObjectClass parent_class;
} DalstonVolumeAppletClass;

GType dalston_volume_applet_get_type (void);

DalstonVolumeApplet *dalston_volume_applet_new (MplPanelClient *client);
GtkWidget *dalston_volume_applet_get_pane (DalstonVolumeApplet *applet);

G_END_DECLS

#endif /* _DALSTON_VOLUME_APPLET */


