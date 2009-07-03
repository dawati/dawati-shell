/*
 * Copyright (c) 2009, Intel Corporation.
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

#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <nbtk/nbtk.h>
#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include "moblin-netbook-netpanel.h"

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
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &standalone, "Do not embed into the mutter-moblin panel", NULL}
};

int
main (int    argc,
      char **argv)
{
  MplPanelClient *client;
  ClutterActor *stage;
  MoblinNetbookNetpanel *netpanel;
  GOptionContext *context;
  GError *error = NULL;

  context = g_option_context_new ("- mutter-moblin myzone panel");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, clutter_get_option_group_without_init ());
  g_option_context_add_group (context, gtk_get_option_group (FALSE));
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical (G_STRLOC ": Error parsing option: %s", error->message);
    g_clear_error (&error);
  }

  g_option_context_free (context);

  MPL_PANEL_CLUTTER_INIT_WITH_GTK (&argc, &argv);

  nbtk_texture_cache_load_cache (nbtk_texture_cache_get_default (),
                                 NBTK_CACHE);
  nbtk_style_load_from_file (nbtk_style_get_default (),
                             THEMEDIR "/panel.css",
                             NULL);

  if (!standalone)
  {
    client = mpl_panel_clutter_new (MPL_PANEL_INTERNET,
                                    _("internet"),
                                    NULL,
                                    "internet-button",
                                    TRUE);

    MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK (client);

    stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));
    netpanel = MOBLIN_NETBOOK_NETPANEL (moblin_netbook_netpanel_new ());
    clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                                 CLUTTER_ACTOR (netpanel));
    clutter_actor_show (CLUTTER_ACTOR (netpanel));
    moblin_netbook_netpanel_set_panel_client (netpanel, client);
    g_signal_connect (client,
                      "set-size",
                      (GCallback)_client_set_size_cb,
                      netpanel);
  } else {
    Window xwin;

    stage = clutter_stage_get_default ();
    clutter_actor_realize (stage);
    xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

    MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK_FOR_XID (xwin);
    netpanel = MOBLIN_NETBOOK_NETPANEL (moblin_netbook_netpanel_new ());
    clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                                 CLUTTER_ACTOR (netpanel));
    clutter_actor_set_size ((ClutterActor *)netpanel, 1016, 500);
    clutter_actor_set_size (stage, 1016, 500);
    clutter_actor_show_all (stage);
  }

  clutter_main ();

  return 0;
}
