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

#include <clutter/clutter.h>
#include <mojito-client/mojito-client.h>
#include <stdlib.h>

#include "penge-block-layout.h"
#include "penge-model-bridge.h"
#include "penge-people-tile.h"
#include "penge-people-model.h"

static MojitoClient *client;
static ClutterModel *model;
static PengeModelBridge *bridge;
static ClutterActor *box;
static ClutterLayoutManager *layout;

static void
_client_open_view_cb (MojitoClient     *client,
                      MojitoClientView *view,
                      gpointer          userdata)
{
  mojito_client_view_start (view);

  model = penge_people_model_new (view);

  penge_model_bridge_set_model (bridge, model);
}

static void
_client_get_services_cb (MojitoClient *client,
                         const GList  *services,
                         gpointer      userdata)
{
  mojito_client_open_view (client,
                           (GList *)services,
                           20,
                           _client_open_view_cb,
                           NULL);
}

static ClutterActor *
factory_func (PengeModelBridge *bridge,
              ClutterContainer *container,
              ClutterModel     *model,
              ClutterModelIter *iter)
{
  ClutterActor *actor;
  MojitoItem *item;

  clutter_model_iter_get (iter, 0, &item, -1);
  actor = g_object_new (PENGE_TYPE_PEOPLE_TILE,
                        "item", item,
                        NULL);

  clutter_actor_set_size (actor, 140, 95);
  clutter_container_add_actor (container,
                               actor);

  if (g_str_equal (item->service, "twitter"))
  {
    clutter_layout_manager_child_set (layout,
                                      container,
                                      actor,
                                      "col-span", 2,
                                      NULL);
  }

  return actor;
}

static void
stage_allocation_changed_cb (ClutterActor           *stage,
                             const ClutterActorBox  *allocation,
                             ClutterAllocationFlags  flags,
                             ClutterActor           *box)
{
  gfloat width, height;

  clutter_actor_box_get_size (allocation, &width, &height);

  clutter_actor_set_size (box, width, height);
}

int
main (int    argc,
      char **argv)
{
  ClutterActor *stage;

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();
  clutter_stage_set_user_resizable (CLUTTER_STAGE (stage), TRUE);
  clutter_actor_set_size (stage, 640, 480);
#if 1
  layout = penge_block_layout_new ();
  penge_block_layout_set_min_tile_size (PENGE_BLOCK_LAYOUT (layout),
                                        140,
                                        92);
  penge_block_layout_set_spacing (PENGE_BLOCK_LAYOUT (layout), 6);

#else
  layout = clutter_flow_layout_new (CLUTTER_FLOW_HORIZONTAL);
#endif

  box = clutter_box_new (layout);

  bridge = g_object_new (PENGE_TYPE_MODEL_BRIDGE, NULL);

  penge_model_bridge_set_container (bridge, CLUTTER_CONTAINER (box));
  penge_model_bridge_set_factory (bridge, factory_func);

  client = mojito_client_new ();
  mojito_client_get_services (client, _client_get_services_cb, NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), box);

  clutter_actor_set_size (box,
                          clutter_actor_get_width (stage),
                          clutter_actor_get_height (stage));
  clutter_actor_show (stage);

  g_signal_connect (stage,
                    "allocation-changed", G_CALLBACK (stage_allocation_changed_cb),
                    box);
  clutter_main ();

  return EXIT_SUCCESS;
}
