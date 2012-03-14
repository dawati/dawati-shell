/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Srinivasa Ragavan <srini@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <locale.h>
#include <string.h>

#include <glib/gi18n.h>

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <dawati-panel/mpl-panel-clutter.h>
#include <dawati-panel/mpl-panel-common.h>
#include <dawati-panel/mpl-entry.h>

#include "mnp-shell.h"

#define WIDGET_SPACING 5
#define ICON_SIZE 48
#define PADDING 8
#define N_COLS 4
#define LAUNCHER_WIDTH  235
#define LAUNCHER_HEIGHT 64

#include <config.h>

static void
stage_size_notify_cb (ClutterActor  *actor,
                      GParamSpec    *pspec,
                      ClutterActor  *launcher)
{
  ClutterActor *stage = clutter_actor_get_stage (launcher);

  clutter_actor_set_size (launcher,
                          clutter_actor_get_width (stage),
                          clutter_actor_get_height (stage));
}

static void
_client_activated_cb  (MnpShell *shell,
                       gpointer       userdata)
{
  mpl_panel_client_hide ((MplPanelClient *)userdata);
}

static gboolean standalone = FALSE;

static GOptionEntry entries[] = {
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &standalone, "Do not embed into the mutter-dawati panel", NULL},
  { NULL }
};

int
main (int    argc,
      char **argv)
{
  MplPanelClient *client;
  ClutterActor *stage;
  ClutterActor *datetime;
  GOptionContext *context;
  GError *error = NULL;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  context = g_option_context_new ("- mutter-dawati date/time panel");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, clutter_get_option_group_without_init ());
  g_option_context_add_group (context, gtk_get_option_group (FALSE));
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical (G_STRLOC ": Error parsing option: %s", error->message);
    g_clear_error (&error);
  }

  g_option_context_free (context);

  mpl_panel_clutter_init_with_gtk (&argc, &argv);

  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/date-panel.css", NULL);

  if (!standalone)
  {
    client = mpl_panel_clutter_new ("datetime",
                                    _("Date/Time"),
                                    NULL,
                                    "datetime-button",
                                    TRUE);

    mpl_panel_clutter_setup_events_with_gtk (MPL_PANEL_CLUTTER (client));
    mpl_panel_client_set_size_request (client, -1, 530);

    stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));

    datetime = mnp_shell_new (stage);
    mnp_shell_set_panel_client (MNP_SHELL (datetime), client);

    g_signal_connect (datetime,
		      "activated",
		      (GCallback)_client_activated_cb,
		      client);

  } else {
    Window xwin;

    stage = clutter_stage_new ();
    clutter_actor_realize (stage);
    xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

    mpl_panel_clutter_setup_events_with_gtk_for_xid (xwin);
    datetime = mnp_shell_new (stage);

    clutter_actor_set_size (stage, 1016, 530);

    clutter_actor_show (stage);
  }

  g_signal_connect (datetime, "parent-set",
                    G_CALLBACK (stage_size_notify_cb), datetime);
  g_signal_connect (stage, "notify::width",
                    G_CALLBACK (stage_size_notify_cb), datetime);
  g_signal_connect (stage, "notify::height",
                    G_CALLBACK (stage_size_notify_cb), datetime);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               (ClutterActor *)datetime);

  clutter_main ();

  return 0;
}

