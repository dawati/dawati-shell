/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Danielle Madeley <danielle.madeley@collabora.co.uk>
 * Derived from people panel, author: Rob Bradford <rob@linux.intel.com>
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

#include <locale.h>
#include <glib/gi18n.h>

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "mnb-home-panel.h"

#include <config.h>

#define WIDTH 1024
#define HEIGHT 580

static void
_client_set_size_cb (MplPanelClient *client,
                     guint           width,
                     guint           height,
                     gpointer        userdata)
{
  g_debug (G_STRLOC ": %d %d", width, height);
  clutter_actor_set_size ((ClutterActor *)userdata,
                          width,
                          height);
}

static gboolean standalone = FALSE;

static GOptionEntry entries[] = {
  { "standalone", 's', 0, G_OPTION_ARG_NONE, &standalone,
    "Do not embed into the mutter-dawati panel", NULL },
  { NULL }
};

int
main (int    argc,
      char **argv)
{
  MplPanelClient *client;
  ClutterActor *stage;
  ClutterActor *home_panel;
  GOptionContext *context;
  GError *error = NULL;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  context = g_option_context_new ("- mutter-dawati home panel");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, clutter_get_option_group_without_init ());
  g_option_context_add_group (context, cogl_get_option_group ());
  g_option_context_add_group (context, gtk_get_option_group (FALSE));
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical (G_STRLOC ": Error parsing option: %s", error->message);
    g_clear_error (&error);
  }

  g_option_context_free (context);

  mpl_panel_clutter_init_with_gtk (&argc, &argv);

  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/panel.css", NULL);

  if (!standalone)
  {
    client = mpl_panel_clutter_new (MPL_PANEL_HOME,
                                    _("home"),
                                    NULL,
                                    "home-button",
                                    TRUE);

    mpl_panel_clutter_setup_events_with_gtk (MPL_PANEL_CLUTTER (client));

    mpl_panel_client_set_size_request (client, WIDTH, HEIGHT);

    stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));
    home_panel = mnb_home_panel_new (client);
    g_signal_connect (client,
                      "size-changed",
                      (GCallback)_client_set_size_cb,
                      home_panel);
  } else {
    Window xwin;

    stage = clutter_stage_new ();
    clutter_actor_realize (stage);
    xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

    mpl_panel_clutter_setup_events_with_gtk_for_xid (xwin);
    home_panel = mnb_home_panel_new (NULL);
    clutter_actor_set_size ((ClutterActor *) home_panel, WIDTH, HEIGHT);
    clutter_actor_set_size (stage, WIDTH, HEIGHT);
    clutter_actor_show_all (stage);
  }

  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               (ClutterActor *) home_panel);


  clutter_main ();

  return 0;
}
