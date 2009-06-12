/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Robert Staudinger <robertx.staudinger@intel.com>
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

#include <stdlib.h>
#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include <libmnb/mnb-panel-clutter.h>
#include <libmnb/mnb-panel-common.h>
#include <gtk/gtk.h>
#include "moblin-netbook-launcher.h"

#define DEFAULT_WIDTH   790
#define DEFAULT_HEIGHT  400

static void
stage_width_notify_cb (ClutterActor  *stage,
                       GParamSpec    *pspec,
                       MnbLauncher   *launcher)
{

}

static void
stage_height_notify_cb (ClutterActor  *stage,
                        GParamSpec    *pspec,
                        MnbLauncher   *launcher)
{

}

static void
launcher_activated_cb (MnbLauncher    *launcher,
                       const gchar    *desktop_file,
                       MnbPanelClient *panel)
{
  mnb_panel_client_launch_application_from_desktop_file (panel,
                                                         desktop_file,
                                                         NULL,
                                                         FALSE,
                                                         -2);
  mnb_panel_client_request_hide (panel);
}

int
main (int     argc,
      char  **argv)
{
  MnbPanelClient  *panel;
  ClutterActor    *stage;
  ClutterActor    *launcher;

  clutter_init (&argc, &argv);
  gtk_init (&argc, &argv);

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             MUTTER_MOBLIN_CSS, NULL);

  /* TODO: split up the CSS, or do we need one at all here,
   * just for the panel button? */
  panel = mnb_panel_clutter_new (MNB_PANEL_APPLICATIONS,
                                  _("applications"),
                                 MUTTER_MOBLIN_CSS,
                                 "applications-button",
                                 TRUE);

  launcher = mnb_launcher_new (DEFAULT_WIDTH, DEFAULT_HEIGHT);
  g_signal_connect (launcher, "launcher-activated",
                    G_CALLBACK (launcher_activated_cb), panel);

  stage = mnb_panel_clutter_get_stage (MNB_PANEL_CLUTTER (panel));
  g_signal_connect (stage, "notify::width",
                    G_CALLBACK (stage_width_notify_cb), launcher);
  g_signal_connect (stage, "notify::height",
                    G_CALLBACK (stage_height_notify_cb), launcher);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), launcher);

  clutter_main ();

  return EXIT_SUCCESS;
}

