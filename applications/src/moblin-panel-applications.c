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
#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include <libmnb/mnb-panel-clutter.h>
#include <libmnb/mnb-panel-common.h>
#include <gtk/gtk.h>
#include "moblin-netbook-launcher.h"
#include "config.h"

static void
stage_width_notify_cb (ClutterActor  *stage,
                       GParamSpec    *pspec,
                       MnbLauncher   *launcher)
{
  guint width = clutter_actor_get_width (stage);

  clutter_actor_set_width (CLUTTER_ACTOR (launcher), width);
}

static void
stage_height_notify_cb (ClutterActor  *stage,
                        GParamSpec    *pspec,
                        MnbLauncher   *launcher)
{
  guint height = clutter_actor_get_height (stage);

  clutter_actor_set_height (CLUTTER_ACTOR (launcher), height);
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

static void
setup_embedded (void)
{
  MnbPanelClient  *panel;
  ClutterActor    *stage;
  ClutterActor    *launcher;

  /* TODO: split up the CSS, or do we need one at all here,
   * just for the panel button? */
  panel = mnb_panel_clutter_new (MNB_PANEL_APPLICATIONS,
                                  _("applications"),
                                 MUTTER_MOBLIN_CSS,
                                 "applications-button",
                                 TRUE);

  launcher = mnb_launcher_new ();
  g_signal_connect (launcher, "launcher-activated",
                    G_CALLBACK (launcher_activated_cb), panel);

  stage = mnb_panel_clutter_get_stage (MNB_PANEL_CLUTTER (panel));
  g_signal_connect (stage, "notify::width",
                    G_CALLBACK (stage_width_notify_cb), launcher);
  g_signal_connect (stage, "notify::height",
                    G_CALLBACK (stage_height_notify_cb), launcher);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), launcher);
}

static void
setup_standalone (void)
{
  ClutterActor  *stage;
  ClutterActor  *launcher;

  stage = clutter_stage_get_default ();

  launcher = mnb_launcher_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), launcher);

  g_signal_connect (stage, "notify::width",
                    G_CALLBACK (stage_width_notify_cb), launcher);
  g_signal_connect (stage, "notify::height",
                    G_CALLBACK (stage_height_notify_cb), launcher);

  clutter_actor_set_size (stage, 1024, 768);
  clutter_actor_show (stage);
}

static gboolean _standalone = FALSE;

static GOptionEntry _options[] = {
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &_standalone, "Do not embed into the mutter-moblin panel", NULL}
};

int
main (int     argc,
      char  **argv)
{
  GOptionContext  *context;
  GError          *error = NULL;

  context = g_option_context_new ("- Mutter-moblin application launcher panel");
  g_option_context_add_main_entries (context, _options, GETTEXT_PACKAGE);
  g_option_context_add_group (context, clutter_get_option_group ());
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_critical ("%s %s", G_STRLOC, error->message);
      g_critical ("Starting in standalone mode.");
      g_clear_error (&error);
      _standalone = TRUE;
    }
  g_option_context_free (context);

  clutter_init (&argc, &argv);
  gtk_init (&argc, &argv);

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             MUTTER_MOBLIN_CSS, NULL);

  if (_standalone)
    setup_standalone ();
  else
    setup_embedded ();

  clutter_main ();

  return EXIT_SUCCESS;
}

