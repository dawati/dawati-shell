
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
#include "mpd-folder-button.h"

static void
_folder_clicked_cb (MpdFolderButton *button,
                    gpointer         user_data)
{
  g_debug ("%s", mpd_folder_button_get_uri (button));
}

int
main (int     argc,
      char  **argv)
{
  ClutterActor *stage;
  ClutterActor *button;

  clutter_init (&argc, &argv);

  stage = clutter_stage_new ();

  button = g_object_new (MPD_TYPE_FOLDER_BUTTON,
                         "label", "Documents",
                         "uri", "file:///home/robsta/Documents",
                         NULL);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (_folder_clicked_cb), NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), button);

  clutter_actor_show_all (stage);
  clutter_main ();
  clutter_actor_destroy (stage);
  return EXIT_SUCCESS;
}
