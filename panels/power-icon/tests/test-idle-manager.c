
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
#include <gtk/gtk.h>
#include <mx/mx.h>
#include "mpd-idle-manager.h"

static void
_lock_button_clicked_cb (MxButton       *button,
                         MpdIdleManager *idlr)
{
  GError *error = NULL;

  mpd_idle_manager_activate_screensaver (idlr, &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
}

static void
_suspend_button_clicked_cb (MxButton       *button,
                            MpdIdleManager *idlr)
{
  GError *error = NULL;

  mpd_idle_manager_suspend (idlr, &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
}

int
main (int     argc,
      char  **argv)
{
  MpdIdleManager  *idlr;
  ClutterActor    *stage;
  ClutterActor    *box;
  ClutterActor    *button;

  clutter_init (&argc, &argv);
  /* Needed for egg-idletime's X extensions. */
  gtk_init (&argc, &argv);

  idlr = mpd_idle_manager_new ();

  stage = clutter_stage_get_default ();

  box = mx_box_layout_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), box);

  button = mx_button_new_with_label ("Activate screensaver");
  g_signal_connect (button, "clicked",
                    G_CALLBACK (_lock_button_clicked_cb), idlr);
  clutter_container_add_actor (CLUTTER_CONTAINER (box), button);

  button = mx_button_new_with_label ("Suspend");
  g_signal_connect (button, "clicked",
                    G_CALLBACK (_suspend_button_clicked_cb), idlr);
  clutter_container_add_actor (CLUTTER_CONTAINER (box), button);

  clutter_actor_show_all (stage);
  clutter_main ();

  g_object_unref (idlr);

  return EXIT_SUCCESS;
}

