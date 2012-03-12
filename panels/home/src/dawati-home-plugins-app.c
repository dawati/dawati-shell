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

#include "dawati-home-plugins-app.h"

G_DEFINE_INTERFACE (DawatiHomePluginsApp, dawati_home_plugins_app,
    G_TYPE_OBJECT)

static void
dawati_home_plugins_app_default_init (DawatiHomePluginsAppInterface *iface)
{
  g_object_interface_install_property (iface,
      g_param_spec_string ("settings-path",
        "Settings Path",
        "The DConf path where this widget should install its settings",
        NULL,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}

/**
 * dawati_home_plugins_app_init:
 * @self: ourselves
 */
void
dawati_home_plugins_app_init (DawatiHomePluginsApp *self)
{
  DawatiHomePluginsAppInterface *iface;

  g_return_if_fail (DAWATI_IS_HOME_APP_PLUGIN (self));

  iface = DAWATI_HOME_APP_PLUGIN_GET_IFACE (self);

  if (iface->init != NULL)
    iface->init (self);
}

/**
 * dawati_home_plugins_app_deinit:
 * @self: ourselves
 */
void
dawati_home_plugins_app_deinit (DawatiHomePluginsApp *self)
{
  DawatiHomePluginsAppInterface *iface;

  g_return_if_fail (DAWATI_IS_HOME_APP_PLUGIN (self));

  iface = DAWATI_HOME_APP_PLUGIN_GET_IFACE (self);

  if (iface->deinit != NULL)
    iface->deinit (self);
}

/**
 * dawati_home_plugins_app_get_widget:
 * @self: ourselves
 *
 * Returns: (transfer full): a #ClutterActor to display on the panel
 */
ClutterActor *
dawati_home_plugins_app_get_widget (DawatiHomePluginsApp *self)
{
  DawatiHomePluginsAppInterface *iface;

  g_return_val_if_fail (DAWATI_IS_HOME_APP_PLUGIN (self), NULL);

  iface = DAWATI_HOME_APP_PLUGIN_GET_IFACE (self);

  if (iface->get_widget != NULL)
    return iface->get_widget (self);

  g_return_val_if_reached (NULL);
}

/**
 * dawati_home_plugins_app_get_configuration:
 * @self: ourselves
 *
 * Returns: (transfer full): a #ClutterActor for configuring the widget
 */
ClutterActor *
dawati_home_plugins_app_get_configuration (DawatiHomePluginsApp *self)
{
  DawatiHomePluginsAppInterface *iface;

  g_return_val_if_fail (DAWATI_IS_HOME_APP_PLUGIN (self), NULL);

  iface = DAWATI_HOME_APP_PLUGIN_GET_IFACE (self);

  if (iface->get_configuration != NULL)
    return iface->get_configuration (self);

  g_return_val_if_reached (NULL);
}
