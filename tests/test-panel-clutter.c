/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/*
 * Simple test for out of process panels.
 *
 */

#include "../libmnb/mnb-panel-clutter.h"

static void
set_size_cb (MnbPanelClutter *panel, guint width, guint height, gpointer data)
{
  ClutterActor *stage = mnb_panel_clutter_get_stage (panel);

  g_debug ("Setting panel window size to %dx%d", width, height);

  clutter_actor_set_size (stage, width, height);
}

static void
make_window_content (MnbPanelClutter *panel)
{
  ClutterActor *stage = mnb_panel_clutter_get_stage (panel);
  ClutterActor *label;

  label = clutter_text_new_with_text ("Sans 16pt", "This is a clutter panel");
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), label);
}

static void
update_content_cb (MnbPanelClutter *panel, gpointer data)
{
  g_debug ("Making content\n");
  make_window_content (panel);
}


int
main (int argc, char *argv[])
{
  MnbPanelClient *panel;

  clutter_init (&argc, &argv);

  panel = mnb_panel_clutter_new ("/org/moblin/Mnb/TestPanel",
                                 "tray-button-test",
                                 "test",
                                 NULL, NULL);

  g_signal_connect (panel, "show-begin",
                    G_CALLBACK (update_content_cb), NULL);

  clutter_main ();

  return 0;
}
