/*
 *
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
 */

#ifndef _PENGE_MODEL_BRIDGE
#define _PENGE_MODEL_BRIDGE

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define PENGE_TYPE_MODEL_BRIDGE penge_model_bridge_get_type()

#define PENGE_MODEL_BRIDGE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_MODEL_BRIDGE, PengeModelBridge))

#define PENGE_MODEL_BRIDGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_MODEL_BRIDGE, PengeModelBridgeClass))

#define PENGE_IS_MODEL_BRIDGE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_MODEL_BRIDGE))

#define PENGE_IS_MODEL_BRIDGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_MODEL_BRIDGE))

#define PENGE_MODEL_BRIDGE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_MODEL_BRIDGE, PengeModelBridgeClass))

typedef struct {
  GObject parent;
} PengeModelBridge;

typedef struct {
  GObjectClass parent_class;
} PengeModelBridgeClass;

GType penge_model_bridge_get_type (void);

typedef ClutterActor *(* PengeModelBridgeFactoryFunc) (PengeModelBridge *bridge,
                                                       ClutterContainer *container,
                                                       ClutterModel     *model,
                                                       ClutterModelIter *iter);

void penge_model_bridge_set_container (PengeModelBridge *bridge,
                                       ClutterContainer *container);
void penge_model_bridge_set_model (PengeModelBridge *bridge,
                                   ClutterModel     *model);
void penge_model_bridge_set_factory (PengeModelBridge           *bridge,
                                     PengeModelBridgeFactoryFunc f);

G_END_DECLS

#endif /* _PENGE_MODEL_BRIDGE */

