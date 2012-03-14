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

#ifndef __MNB_HOME_PLUGINS_ENGINE_H__
#define __MNB_HOME_PLUGINS_ENGINE_H__

#include <glib-object.h>

#include <libpeas/peas.h>

#include "dawati-home-plugins-app.h"

G_BEGIN_DECLS

#define MNB_TYPE_HOME_PLUGINS_ENGINE	(mnb_home_plugins_engine_get_type ())
#define MNB_HOME_PLUGINS_ENGINE(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_HOME_PLUGINS_ENGINE, MnbHomePluginsEngine))
#define MNB_HOME_PLUGINS_ENGINE_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), MNB_TYPE_HOME_PLUGINS_ENGINE, MnbHomePluginsEngineClass))
#define MNB_IS_HOME_PLUGINS_ENGINE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_HOME_PLUGINS_ENGINE))
#define MNB_IS_HOME_PLUGINS_ENGINE_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), MNB_TYPE_HOME_PLUGINS_ENGINE))
#define MNB_HOME_PLUGINS_ENGINE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_HOME_PLUGINS_ENGINE, MnbHomePluginsEngineClass))

typedef struct _MnbHomePluginsEngine MnbHomePluginsEngine;
typedef struct _MnbHomePluginsEngineClass MnbHomePluginsEngineClass;
typedef struct _MnbHomePluginsEnginePrivate MnbHomePluginsEnginePrivate;

struct _MnbHomePluginsEngine
{
  PeasEngine parent;

  MnbHomePluginsEnginePrivate *priv;
};

struct _MnbHomePluginsEngineClass
{
  PeasEngineClass parent_class;
};

GType mnb_home_plugins_engine_get_type (void);

MnbHomePluginsEngine *mnb_home_plugins_engine_dup (void);

DawatiHomePluginsApp *mnb_home_plugins_engine_create_app (
    MnbHomePluginsEngine *self,
    const char *module,
    const char *settings_path);

G_END_DECLS

#endif
