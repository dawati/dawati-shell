
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Robert Staudinger <robert.staudinger@intel.com>
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
#include <clutter/clutter.h>
#include <mx/mx.h>
#include <meego-panel/mpl-content-pane.h>

int
main (int     argc,
      char  **argv)
{
  ClutterActor *stage;
  ClutterActor *pane;
  ClutterActor *button;

  clutter_init (&argc, &argv);

  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/theme.css", NULL);

  stage = clutter_stage_get_default ();

  pane = mpl_content_pane_new ("Foo");
  clutter_actor_set_size (pane, 480, 320);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), pane);

  button = mx_button_new_with_label ("Bar");
  mpl_content_pane_set_header_actor (MPL_CONTENT_PANE (pane), button);

  button = mx_button_new_with_label ("Baz");
  clutter_container_add_actor (CLUTTER_CONTAINER (pane), button);

  clutter_actor_show_all (stage);
  clutter_main ();

  return EXIT_SUCCESS;
}

