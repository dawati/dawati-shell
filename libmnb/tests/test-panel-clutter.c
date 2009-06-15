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

#include <mnb/mnb-panel-clutter.h>
#include <mnb/mnb-panel-common.h>

static void
make_window_content (MnbPanelClutter *panel)
{
  ClutterActor *stage = mnb_panel_clutter_get_stage (panel);
  ClutterActor *label;
  ClutterColor  white = {0xff, 0xff, 0xff, 0xff};
  ClutterColor  black = {0, 0, 0, 0xff};

  clutter_stage_set_color (CLUTTER_STAGE (stage), &white);

  label = clutter_text_new_with_text ("Sans 16pt", "This is a clutter panel");
  clutter_text_set_color  (CLUTTER_TEXT (label), &black);
  clutter_text_set_editable (CLUTTER_TEXT (label), TRUE);

  clutter_stage_set_key_focus (CLUTTER_STAGE (stage), label);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), label);
}

int
main (int argc, char *argv[])
{
  MnbPanelClient *panel;

  clutter_init (&argc, &argv);

  /*
   * NB: the toolbar service indicates whether this panel requires access
   *     to the API provided by org.moblin.Mnb.Toolbar -- if you need to do
   *     any application launching, etc., then pass TRUE.
   */
  panel = mnb_panel_clutter_new (MNB_PANEL_TEST,           /* the panel slot */
                                 "test",                   /* tooltip */
                                 CSS_DIR"/test-panel.css", /*stylesheet */
                                 "state1",                 /* button style */
                                 FALSE);                /* no toolbar service*/

  /*
   * Strictly speaking, it is not necessary to construct the window contents
   * at this point, the panel can instead hook to MnbPanelClient::set-size
   * signal. However, once the set-size signal has been emitted, the panel
   * window must remain in a state suitable to it being shown.
   *
   * The panel can also hook into the MnbPanelClient::show-begin signal, to be
   * when it is being shown, but this signal is assynchronous, so that the
   * panel might finish showing *before* the Panel handles this signal.
   */
  make_window_content (MNB_PANEL_CLUTTER (panel));

  clutter_main ();

  return 0;
}
