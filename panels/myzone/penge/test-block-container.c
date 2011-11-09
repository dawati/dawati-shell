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

#include <stdlib.h>

#include <penge/penge-block-container.h>

static void
stage_allocation_changed_cb (ClutterActor           *stage,
                             const ClutterActorBox  *allocation,
                             ClutterAllocationFlags  flags,
                             ClutterActor           *block)
{
  gfloat width, height;

  clutter_actor_box_get_size (allocation, &width, &height);

  clutter_actor_set_size (block, width, height);
}

int
main (int    argc,
      char **argv)
{
  ClutterActor *stage;
  ClutterActor *block;
  gint i;
  gint n_rects = 16;

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();
  clutter_stage_set_user_resizable (CLUTTER_STAGE (stage), TRUE);
  clutter_actor_set_size (stage, 640, 480);

  block = penge_block_container_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), block);

  penge_block_container_set_spacing (PENGE_BLOCK_CONTAINER (block), 10);
  penge_block_container_set_min_tile_size (PENGE_BLOCK_CONTAINER (block),
                                           140,
                                           92);

  /* Shamelessly stolen from test-flow-layout from Clutter */
  for (i = 0; i < n_rects; i++)
  {
    ClutterColor color = { 255, 255, 255, 224 };
    ClutterActor *rect;

    clutter_color_from_hls (&color,
                            360.0 / n_rects * i,
                            0.5,
                            0.8);
    rect = clutter_rectangle_new_with_color (&color);

    clutter_container_add_actor (CLUTTER_CONTAINER (block), rect);
    clutter_container_child_set (CLUTTER_CONTAINER (block),
                                 rect,
                                 "col-span", (i % 2) + 1,
                                 NULL);
  }

  clutter_actor_show_all (stage);

  clutter_actor_set_size (block,
                          clutter_actor_get_width (stage),
                          clutter_actor_get_height (stage));
  g_signal_connect (stage,
                    "allocation-changed", G_CALLBACK (stage_allocation_changed_cb),
                    block);
  clutter_main ();

  return EXIT_SUCCESS;
}
