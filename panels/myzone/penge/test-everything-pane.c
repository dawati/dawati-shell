/*
 * Copyright (C) 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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

#include <stdlib.h>

#include "penge-everything-pane.h"


static void
stage_allocation_changed_cb (ClutterActor           *stage,
                             const ClutterActorBox  *allocation,
                             ClutterAllocationFlags  flags,
                             ClutterActor           *pane)
{
  gfloat width, height;

  clutter_actor_box_get_size (allocation, &width, &height);

  clutter_actor_set_size (pane, width, height);
}


int
main (int    argc,
      char **argv)
{
  ClutterActor *stage;
  ClutterActor *pane;

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();
  clutter_stage_set_user_resizable (CLUTTER_STAGE (stage), TRUE);
  clutter_actor_set_size (stage, 640, 480);

  pane = g_object_new (PENGE_TYPE_EVERYTHING_PANE, NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), pane);

  clutter_actor_show_all (stage);

  clutter_actor_set_size (pane,
                          clutter_actor_get_width (stage),
                          clutter_actor_get_height (stage));
  g_signal_connect (stage,
                    "allocation-changed", G_CALLBACK (stage_allocation_changed_cb),
                    pane);
  clutter_main ();

  return EXIT_SUCCESS;
}
