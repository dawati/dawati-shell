/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Emmanuele Bassi <ebassi@linux.intel.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <clutter/clutter.h>
#include <gtk/gtk.h>
#include <clutter/x11/clutter-x11.h>

#include <libsocialweb-client/sw-client.h>
#include <mx/mx.h>

#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>

#include "mps-view-bridge.h"
#include "mps-feed-switcher.h"

#define NEW_PLACEHOLDER_TEXT _("Hello! This is the Status panel." \
"When you have a web account configured" \
"(like Twitter) you will be able to view your" \
"feeds and update your status here")

typedef struct _MoblinStatusPanel
{
  SwClient *client;
  SwClientView *view;
  MplPanelClient *panel_client;
  MpsViewBridge *bridge;
} MoblinStatusPanel;


static MoblinStatusPanel *panel;

void
moblin_status_panel_hide (void)
{
  if (panel->panel_client)
    mpl_panel_client_hide ((MplPanelClient *)panel->panel_client);
  else
    g_debug (G_STRLOC ": Would hide the panel");
}

static void 
_client_view_opened_cb (SwClient     *client,
                        SwClientView *view,
                        gpointer      userdata)
{
  MoblinStatusPanel *status_panel = (MoblinStatusPanel *)userdata;

  mps_view_bridge_set_view (status_panel->bridge, view);
}

static ClutterActor *
make_status (MoblinStatusPanel *status_panel)
{
  ClutterActor *pane;
  ClutterActor *table;
  ClutterActor *label;

  status_panel->client = sw_client_new ();
#if 0
  pane = mps_feed_pane_new (status_panel->client,
                            sw_client_get_service (status_panel->client,
                                                   "twitter"));
#endif

  table = mx_table_new ();
  clutter_actor_set_name (table, "status-panel");
  label = mx_label_new_with_text (_("Status"));
  clutter_actor_set_name (label, "status-panel-header-label");
  mx_table_add_actor_with_properties (MX_TABLE (table),
                      label,
                      0, 0,
                      "x-expand", FALSE,
                      "y-expand", FALSE,
                      NULL);

  pane = mps_feed_switcher_new (status_panel->client);
  mx_table_add_actor_with_properties (MX_TABLE (table),
                      pane,
                      1, 0,
                      "x-expand", TRUE,
                      "y-expand", TRUE,
                      "x-fill", TRUE,
                      "y-fill", TRUE,
                      NULL);
  return table;
}

static void
on_client_set_size (MplPanelClient *client,
                    guint           width,
                    guint           height,
                    ClutterActor   *table)
{
  clutter_actor_set_size (table, width, height);
}

static void
setup_standalone (MoblinStatusPanel *status_panel)
{
  ClutterActor *stage, *status;
  Window xwin;

  status = make_status (status_panel);
  clutter_actor_set_size (status, 1000, 600);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 1000, 600);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), status);

  clutter_actor_realize (stage);
  xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

  MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK_FOR_XID (xwin);

  clutter_actor_show (stage);
}

static void
setup_panel (MoblinStatusPanel *status_panel)
{
  MplPanelClient *panel;
  ClutterActor *stage, *status;

  panel = mpl_panel_clutter_new ("status",
                                 _("status"),
                                 NULL,
                                 "status-button",
                                 TRUE);
  status_panel->panel_client = panel;

  MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK (panel);

  status = make_status (status_panel);
  mpl_panel_client_set_height_request (panel, 600);
  stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (panel));
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), status);

  g_signal_connect (panel,
                    "set-size", G_CALLBACK (on_client_set_size),
                    status);
}

static gboolean status_standalone = FALSE;

static GOptionEntry status_options[] = {
  {
    "standalone", 's',
    0,
    G_OPTION_ARG_NONE, &status_standalone,
    "Do not embed into mutter-moblin", NULL
  },

  { NULL }
};

int
main (int argc, char *argv[])
{
  GOptionContext *context;
  GError *error = NULL;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  context = g_option_context_new ("- moblin status panel");
  g_option_context_add_main_entries (context, status_options, GETTEXT_PACKAGE);
  g_option_context_add_group (context, clutter_get_option_group_without_init ());
  g_option_context_add_group (context, cogl_get_option_group ());
  g_option_context_add_group (context, gtk_get_option_group (FALSE));

  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical (G_STRLOC ": Error parsing option: %s", error->message);
    g_clear_error (&error);
  }

  g_option_context_free (context);

  MPL_PANEL_CLUTTER_INIT_WITH_GTK (&argc, &argv);

  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/panel.css",
                           &error);

  if (error)
    {
      g_critical ("Unable to load style: %s", error->message);
      g_clear_error (&error);
    }

  panel = g_new0 (MoblinStatusPanel, 1);

  if (status_standalone)
    setup_standalone (panel);
  else
    setup_panel (panel);

  clutter_main ();


  return EXIT_SUCCESS;
}

/* vim:set ts=8 expandtab */
