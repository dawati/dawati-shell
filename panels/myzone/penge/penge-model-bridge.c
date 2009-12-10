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

#include "penge-model-bridge.h"

G_DEFINE_TYPE (PengeModelBridge, penge_model_bridge, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_MODEL_BRIDGE, PengeModelBridgePrivate))

typedef struct _PengeModelBridgePrivate PengeModelBridgePrivate;

struct _PengeModelBridgePrivate {
  ClutterModel *model;
  PengeModelBridgeFactoryFunc factory;
  ClutterContainer *container;
  GHashTable *pointer_to_actor;
};

static void
penge_model_bridge_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_model_bridge_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_model_bridge_dispose (GObject *object)
{
  PengeModelBridgePrivate *priv = GET_PRIVATE (object);

  if (priv->model)
  {
    g_object_unref (priv->model);
    priv->model = NULL;
  }

  if (priv->container)
  {
    g_object_unref (priv->container);
    priv->container = NULL;
  }

  if (priv->pointer_to_actor)
  {
    g_hash_table_unref (priv->pointer_to_actor);
  }

  G_OBJECT_CLASS (penge_model_bridge_parent_class)->dispose (object);
}

static void
penge_model_bridge_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_model_bridge_parent_class)->finalize (object);
}

static void
penge_model_bridge_class_init (PengeModelBridgeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeModelBridgePrivate));

  object_class->get_property = penge_model_bridge_get_property;
  object_class->set_property = penge_model_bridge_set_property;
  object_class->dispose = penge_model_bridge_dispose;
  object_class->finalize = penge_model_bridge_finalize;
}

static void
penge_model_bridge_init (PengeModelBridge *self)
{
  PengeModelBridgePrivate *priv = GET_PRIVATE (self);

  priv->pointer_to_actor = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static void
penge_model_bridge_resync (PengeModelBridge *bridge)
{
  PengeModelBridgePrivate *priv = GET_PRIVATE (bridge);
  GList *children, *l;
  ClutterModelIter *iter;
  gpointer p;
  ClutterActor *actor;

  children = clutter_container_get_children (priv->container);
  iter = clutter_model_get_first_iter (priv->model);

  while (iter && !clutter_model_iter_is_last (iter))
  {
    clutter_model_iter_get (iter, 0, &p, -1);

    actor = g_hash_table_lookup (priv->pointer_to_actor, p);

    if (!actor)
    {
      actor = priv->factory (bridge, priv->container, priv->model, iter);
      g_hash_table_insert (priv->pointer_to_actor, p, actor);

      g_debug (G_STRLOC ": Creating new actor");

    } else {
      g_debug (G_STRLOC ": Reordering after last actor");
      children = g_list_remove (children, actor);
    }

    clutter_container_lower_child (priv->container,
                                   actor,
                                   NULL);

    iter = clutter_model_iter_next (iter);
  }

  for (l = children; l; l = l->next)
  {
    clutter_container_remove_actor (priv->container,
                                    CLUTTER_ACTOR (l->data));
    g_hash_table_remove (priv->pointer_to_actor, l->data);
  }

  g_list_free (children);
}

void
penge_model_bridge_set_container (PengeModelBridge *bridge,
                                  ClutterContainer *container)
{
  PengeModelBridgePrivate *priv = GET_PRIVATE (bridge);

  /* TODO: Handle if container already set */
  priv->container = g_object_ref (container);
}

static void
_model_bulk_end_cb (ClutterModel *model,
                    gpointer      userdata)
{
  PengeModelBridge *bridge = (PengeModelBridge *)userdata;

  penge_model_bridge_resync (bridge);
}

void
penge_model_bridge_set_model (PengeModelBridge *bridge,
                              ClutterModel     *model)
{
  PengeModelBridgePrivate *priv = GET_PRIVATE (bridge);

  /* TODO: Handle if model already set */
  priv->model = g_object_ref (model);

  g_signal_connect (priv->model,
                    "bulk-end", (GCallback)_model_bulk_end_cb,
                    bridge);

  penge_model_bridge_resync (bridge);
}

void
penge_model_bridge_set_factory (PengeModelBridge           *bridge,
                                PengeModelBridgeFactoryFunc f)
{
  PengeModelBridgePrivate *priv = GET_PRIVATE (bridge);

  /* TODO: Handle if factory already set */
  priv->factory = f;
}


