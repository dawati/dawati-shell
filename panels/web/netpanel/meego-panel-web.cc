/*
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA. 
 */

#include <locale.h>
#include <string>
#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <mx/mx.h>
#include <meego-panel/mpl-panel-clutter.h>
#include <meego-panel/mpl-panel-common.h>

#include "meego-netbook-netpanel.h"

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

static gboolean
stage_button_press_event (ClutterStage          *stage,
                          ClutterEvent          *event,
                          MeegoNetbookNetpanel *netpanel)
{
  meego_netbook_netpanel_button_press (netpanel);
  return TRUE;
}

static gboolean
stage_delete_event (ClutterStage *stage,
                    ClutterEvent *event,
                    gpointer* userdata)
{
  return TRUE;
}


static gboolean standalone = FALSE;

static GOptionEntry entries[] = {
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &standalone, "Do not embed into the mutter-meego panel", NULL}
};


int
main (int    argc,
      char **argv)
{
  MplPanelClient *client;
  ClutterActor *stage;
  MeegoNetbookNetpanel *netpanel;
  GOptionContext *context;
  std::string browser_name = "chromium";
  GError *error = NULL;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  context = g_option_context_new ("- mutter-meego myzone panel");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, clutter_get_option_group_without_init ());
  g_option_context_add_group (context, gtk_get_option_group (FALSE));
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical (G_STRLOC ": Error parsing option: %s", error->message);
    g_clear_error (&error);
  }

  g_option_context_free (context);

  g_thread_init(NULL);
  clutter_threads_init();

  mpl_panel_clutter_init_with_gtk (&argc, &argv);

  mx_texture_cache_load_cache (mx_texture_cache_get_default (),
                                 MX_CACHE);
  mx_style_load_from_file (mx_style_get_default (),
                             THEMEDIR "/panel.css",
                             NULL);

  if (!standalone)
  {
    client = mpl_panel_clutter_new (MPL_PANEL_INTERNET,
                                    _("internet"),
                                    NULL,
                                    "internet-button",
                                    TRUE);


    mpl_panel_clutter_setup_events_with_gtk (MPL_PANEL_CLUTTER(client));

    stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));
    netpanel = MEEGO_NETBOOK_NETPANEL (meego_netbook_netpanel_new ());
    meego_netbook_netpanel_set_browser(netpanel, browser_name.c_str());

    ClutterActor  *content_pane;
    ClutterActor  *base_pane;
    ClutterActor  *label;

    base_pane = mx_box_layout_new();
    clutter_actor_set_name (base_pane, "base-pane");
    mx_box_layout_set_orientation (MX_BOX_LAYOUT (base_pane), MX_ORIENTATION_VERTICAL);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), base_pane);

    label = mx_label_new_with_text (_("Internet"));
    clutter_actor_set_name (label, "panel-label");
    clutter_container_add_actor (CLUTTER_CONTAINER (base_pane), label);

    content_pane = mx_box_layout_new ();
    clutter_actor_set_name (content_pane, "pane");
    mx_box_layout_set_orientation (MX_BOX_LAYOUT (content_pane), MX_ORIENTATION_VERTICAL);
    clutter_container_add_actor (CLUTTER_CONTAINER (base_pane), content_pane);
    clutter_container_child_set (CLUTTER_CONTAINER (base_pane), content_pane,
                                 "expand", TRUE,
                                 "x-fill", TRUE,
                                 "y-fill", TRUE,
                                 NULL);

    clutter_container_add_actor (CLUTTER_CONTAINER (content_pane),
                                 CLUTTER_ACTOR (netpanel));
    clutter_container_child_set (CLUTTER_CONTAINER (content_pane), CLUTTER_ACTOR (netpanel),
                                 "expand", TRUE,
                                 "x-fill", TRUE,
                                 "y-fill", TRUE,
                                 NULL);

    meego_netbook_netpanel_set_panel_client (netpanel, client);
    g_signal_connect (client,
                      "set-size",
                      (GCallback)_client_set_size_cb,
                      base_pane);
  } else {
    Window xwin;
    ClutterActor  *content_pane;
    ClutterActor  *base_pane;
    ClutterActor  *label;

    stage = clutter_stage_get_default ();
    clutter_actor_realize (stage);
    xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

    mpl_panel_clutter_setup_events_with_gtk_for_xid (xwin);
    netpanel = MEEGO_NETBOOK_NETPANEL (meego_netbook_netpanel_new ());
    meego_netbook_netpanel_set_browser(netpanel, browser_name.c_str());

    base_pane = mx_box_layout_new();
    clutter_actor_set_name (base_pane, "base-pane");
    mx_box_layout_set_orientation (MX_BOX_LAYOUT (base_pane), MX_ORIENTATION_VERTICAL);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), base_pane);

    label = mx_label_new_with_text (_("Internet"));
    clutter_actor_set_name (label, "panel-label");
    clutter_container_add_actor (CLUTTER_CONTAINER (base_pane), label);

    content_pane = mx_box_layout_new ();
    clutter_actor_set_name (content_pane, "pane");
    mx_box_layout_set_orientation (MX_BOX_LAYOUT (content_pane), MX_ORIENTATION_VERTICAL);
    clutter_container_add_actor (CLUTTER_CONTAINER (base_pane), content_pane);
    clutter_container_child_set (CLUTTER_CONTAINER (base_pane), content_pane,
                                 "expand", TRUE,
                                 "x-fill", TRUE,
                                 "y-fill", TRUE,
                                 NULL);

    clutter_container_add_actor (CLUTTER_CONTAINER (content_pane),
                                 CLUTTER_ACTOR (netpanel));
    clutter_container_child_set (CLUTTER_CONTAINER (content_pane), CLUTTER_ACTOR (netpanel),
                                 "expand", TRUE,
                                 "x-fill", TRUE,
                                 "y-fill", TRUE,
                                 NULL);
    clutter_actor_set_size (stage, 1016, 500);
    clutter_actor_set_size (base_pane, 1016, 500);
    clutter_actor_show (CLUTTER_ACTOR (netpanel));
    clutter_actor_show_all (stage);

    g_signal_connect (stage,
                    "delete-event",
                    (GCallback)stage_delete_event,
                    NULL);
  }

  g_signal_connect (stage,
                    "button-press-event",
                    (GCallback)stage_button_press_event,
                    netpanel);

  clutter_main ();

  return 0;
}
