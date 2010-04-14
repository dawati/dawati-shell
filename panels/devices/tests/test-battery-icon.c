
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

#include <stdbool.h>
#include <stdlib.h>
#include <clutter/clutter.h>
#include <gtk/gtk.h>
#include <mx/mx.h>
#include "mpd-battery-icon.h"

typedef struct
{
  ClutterActor  *icon;
  GList         *frames;
} TestBatteryIcon;

static void
_button_clicked_cb (MxButton        *button,
                    TestBatteryIcon *app)
{
  mpd_battery_icon_animate (MPD_BATTERY_ICON (app->icon), app->frames);
}

int
main (int     argc,
      char  **argv)
{
  ClutterActor    *stage;
  ClutterActor    *vbox;
  ClutterActor    *button;
  TestBatteryIcon  app;
  GError          *error = NULL;

  clutter_init (&argc, &argv);
  /* Just for icon theme, no widgets. */
  gtk_init (&argc, &argv);

  stage = clutter_stage_get_default ();
  vbox = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (vbox), MX_ORIENTATION_VERTICAL);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), vbox);

  app.icon = mpd_battery_icon_new ();
  clutter_texture_set_sync_size (CLUTTER_TEXTURE (app.icon), true);
  clutter_texture_set_from_file (CLUTTER_TEXTURE (app.icon),
                                 PKGICONDIR"/battery-icon-plugged.png",
                                 &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
    return EXIT_FAILURE;
  }
  mpd_battery_icon_set_fps (MPD_BATTERY_ICON (app.icon), 5);
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), app.icon);

  button = mx_button_new_with_label ("Animate");
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), button);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (_button_clicked_cb), &app);

  app.frames = mpd_battery_icon_load_frames_from_dir (
                PKGICONDIR"/battery-icon-plugged",
                &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
    return EXIT_FAILURE;
  }

  clutter_actor_show_all (stage);
  clutter_main ();

  g_list_foreach (app.frames, (GFunc) g_object_unref, NULL);
  g_list_free (app.frames);

  return EXIT_SUCCESS;
}

