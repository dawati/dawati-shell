/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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
#include <mx/mx.h>
#include <gio/gdesktopappinfo.h>
#include <dawati-panel/mpl-panel-clutter.h>
#include <dawati-panel/mpl-panel-common.h>
#include "mnb-people-panel.h"

#include <config.h>

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
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &standalone, "Do not embed into the mutter-dawati panel", NULL},
  { NULL }
};

static void
start_empathy (void)
{
  GDesktopAppInfo *app_info;
  GError *error = NULL;
  const gchar *args[3] = { NULL, };

  app_info = g_desktop_app_info_new ("empathy.desktop");
  args[0] = g_app_info_get_commandline (G_APP_INFO (app_info));
  args[1] = "--start-hidden";
  args[2] = NULL;

  if (!g_spawn_async (NULL,
                      (gchar **)args,
                      NULL,
                      G_SPAWN_SEARCH_PATH,
                      NULL,
                      NULL,
                      NULL,
                      &error))
  {
    g_warning (G_STRLOC ": Error starting empathy: %s", error->message);
    g_clear_error (&error);
  }
}

int
main (int    argc,
      char **argv)
{
  MplPanelClient *client;
  ClutterActor *stage;
  ClutterActor *people_panel;
  GOptionContext *context;
  GError *error = NULL;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  context = g_option_context_new ("- mutter-dawati people panel");
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
    /* Ensure that empathy is running. This is mandatory since that process is
     * responsible to show notifications and some sub-dialogs like new-contact
     * or join-room. Will be replaced later... */
    start_empathy ();

    client = mpl_panel_clutter_new (MPL_PANEL_PEOPLE,
                                    _("people"),
                                    NULL,
                                    "people-button",
                                    TRUE);

    mpl_panel_clutter_setup_events_with_gtk (MPL_PANEL_CLUTTER (client));

    mpl_panel_client_set_size_request (client, 1024, 580);

    stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));
    people_panel = mnb_people_panel_new ();
    mnb_people_panel_set_panel_client (MNB_PEOPLE_PANEL (people_panel), client);
    g_signal_connect (client,
                      "size-changed",
                      (GCallback)_client_set_size_cb,
                      people_panel);
  } else {
    Window xwin;

    stage = clutter_stage_new ();
    clutter_actor_realize (stage);
    xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

    mpl_panel_clutter_setup_events_with_gtk_for_xid (xwin);
    people_panel = mnb_people_panel_new ();
    clutter_actor_set_size ((ClutterActor *)people_panel, 1024, 580);
    clutter_actor_set_size (stage, 1024, 580);
    clutter_actor_show_all (stage);
  }

  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               (ClutterActor *)people_panel);


  clutter_main ();

  return 0;
}
