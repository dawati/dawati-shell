/*
 * Copyright (c) 2011 Intel Corp.
 *
 * Author: Danielle Madeley <danielle.madeley@collabora.co.uk>
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

#ifndef __DAWATI_HOME_APP_PLUGIN__
#define __DAWATI_HOME_APP_PLUGIN__

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define DAWATI_HOME_PLUGINS_TYPE_APP    (dawati_home_plugins_app_get_type ())
#define DAWATI_HOME_PLUGINS_APP(o)       (G_TYPE_CHECK_INSTANCE_CAST ((o), \
      DAWATI_HOME_PLUGINS_TYPE_APP, DawatiHomePluginsApp))
#define DAWATI_HOME_APP_PLUGIN_IFACE(i) (G_TYPE_CHECK_CLASS_CAST ((i), \
      DAWATI_HOME_PLUGINS_TYPE_APP, DawatiHomePluginsAppInterface))
#define DAWATI_IS_HOME_APP_PLUGIN(o)    (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
      DAWATI_HOME_PLUGINS_TYPE_APP))
#define DAWATI_HOME_APP_PLUGIN_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE((o),\
      DAWATI_HOME_PLUGINS_TYPE_APP, DawatiHomePluginsAppInterface))

typedef struct _DawatiHomePluginsApp DawatiHomePluginsApp;
typedef struct _DawatiHomePluginsAppInterface DawatiHomePluginsAppInterface;

struct _DawatiHomePluginsAppInterface
{
  GTypeInterface parent_iface;

  ClutterActor * (* get_widget) (DawatiHomePluginsApp *self);
  ClutterActor * (* get_configuration) (DawatiHomePluginsApp *self);
};

GType dawati_home_plugins_app_get_type (void) G_GNUC_CONST;

ClutterActor *dawati_home_plugins_app_get_widget (DawatiHomePluginsApp *self);
ClutterActor *dawati_home_plugins_app_get_configuration (DawatiHomePluginsApp *self);

G_END_DECLS

#endif
