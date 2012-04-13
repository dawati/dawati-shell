/*
 * Copyright (c) 2012 Intel Corp.
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

#include <girepository.h>

#include "mnb-home-plugins-engine.h"
#include "utils.h"

G_DEFINE_TYPE (MnbHomePluginsEngine, mnb_home_plugins_engine, PEAS_TYPE_ENGINE);

struct _MnbHomePluginsEnginePrivate
{
  int dummy;
};

GObject *
mnb_home_plugins_engine_constructor (GType type,
    guint n_props,
    GObjectConstructParam *props)
{
  static GObject *singleton = NULL;

  if (singleton != NULL)
    {
      return g_object_ref (singleton);
    }
  else
    {
      singleton = G_OBJECT_CLASS (mnb_home_plugins_engine_parent_class)->constructor (type, n_props, props);

      g_object_add_weak_pointer (singleton, (gpointer) &singleton);

      return singleton;
    }
}

static void
mnb_home_plugins_engine_class_init (MnbHomePluginsEngineClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor = mnb_home_plugins_engine_constructor;

  g_type_class_add_private (gobject_class,
      sizeof (MnbHomePluginsEnginePrivate));
}

static void
mnb_home_plugins_engine_init (MnbHomePluginsEngine *self)
{
  const char *plugins_dir;
  GError *error = NULL;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      MNB_TYPE_HOME_PLUGINS_ENGINE, MnbHomePluginsEnginePrivate);

  peas_engine_enable_loader (PEAS_ENGINE (self), "gjs");

  plugins_dir = g_getenv ("DAWATI_HOME_PLUGINS_DIR");

  if (STR_EMPTY (plugins_dir))
    plugins_dir = PLUGINS_DIR;

  DEBUG ("Loading plugins from '%s'", plugins_dir);

  peas_engine_add_search_path (PEAS_ENGINE (self), plugins_dir, NULL);

  /* FIXME: only set this when required */
  g_irepository_prepend_search_path (".");

  if (!g_irepository_require (g_irepository_get_default (),
        "Peas", "1.0", 0, &error))
    {
      g_critical ("Could not load Peas repository: %s", error->message);
      g_error_free (error);
      return;
    }

  if (!g_irepository_require (g_irepository_get_default (),
        "DawatiHomePlugins", "1.0", 0, &error))
    {
      g_critical ("Could not load DawatiHomePlugins repository: %s",
          error->message);
      g_error_free (error);
      return;
    }
}

MnbHomePluginsEngine *
mnb_home_plugins_engine_dup (void)
{
  MnbHomePluginsEngine *self = g_object_new (MNB_TYPE_HOME_PLUGINS_ENGINE,
      NULL);

  peas_engine_rescan_plugins (PEAS_ENGINE (self));

  return self;
}

DawatiHomePluginsApp *
mnb_home_plugins_engine_create_app (MnbHomePluginsEngine *self,
    const char *module,
    const char *settings_path)
{
  PeasPluginInfo *plugin_info;

  g_return_val_if_fail (MNB_IS_HOME_PLUGINS_ENGINE (self), NULL);

  plugin_info = peas_engine_get_plugin_info (PEAS_ENGINE (self), module);

  g_return_val_if_fail (plugin_info != NULL, NULL);

  DEBUG ("Loading plugin '%s' from '%s'",
      peas_plugin_info_get_module_name (plugin_info),
      peas_plugin_info_get_module_dir (plugin_info));

  if (!peas_engine_load_plugin (PEAS_ENGINE (self), plugin_info))
    {
      g_critical ("Failed to load plugin '%s'", module);
      return NULL;
    }

  return DAWATI_HOME_PLUGINS_APP (peas_engine_create_extension (
        PEAS_ENGINE (self),
        plugin_info,
        DAWATI_HOME_PLUGINS_TYPE_APP,
        "settings-path", settings_path,
        NULL));
}
