
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

#include <locale.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>
#include <mx/mx.h>
#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include <libnotify/notify.h>
#include <dalston/dalston-button-monitor.h>
#include "mpd-computer-pane.h"
#include "mpd-folder-pane.h"
#include "config.h"

static void
_pane_request_hide_cb (ClutterActor   *pane,
                       MplPanelClient *panel)
{
  if (panel)
    mpl_panel_client_hide (panel);
}

static void
stage_width_notify_cb (ClutterActor  *stage,
                       GParamSpec    *pspec,
                       ClutterActor  *content)
{
  guint width = clutter_actor_get_width (stage);

  clutter_actor_set_width (CLUTTER_ACTOR (content), width);
}

static void
stage_height_notify_cb (ClutterActor  *stage,
                        GParamSpec    *pspec,
                        ClutterActor  *content)
{
  guint height = clutter_actor_get_height (stage);

  clutter_actor_set_height (CLUTTER_ACTOR (content), height);
}

static void
panel_set_size_cb (MplPanelClient *panel,
                   guint           width,
                   guint           height,
                   ClutterActor   *content)
{
  clutter_actor_set_size (CLUTTER_ACTOR (content), width, height);
}

static ClutterActor *
create_panel (MplPanelClient *panel)
{
  ClutterActor *box;
  ClutterActor *pane;

  box = mx_box_layout_new ();

  pane = mpd_folder_pane_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (box), pane);

  pane = mpd_computer_pane_new ();
  g_signal_connect (pane, "request-hide",
                    G_CALLBACK (_pane_request_hide_cb), panel);
  clutter_container_add_actor (CLUTTER_CONTAINER (box), pane);

  return box;
}

int
main (int     argc,
      char  **argv)
{
  static gboolean _standalone = FALSE;
  static GOptionEntry _options[] = {
    { "standalone", 's', 0, G_OPTION_ARG_NONE, &_standalone, "Do not embed into the mutter-moblin panel", NULL },
    { NULL }
  };

  DalstonButtonMonitor *button_monitor;
  ClutterActor    *stage;
  ClutterActor    *content;
  GOptionContext  *context;
  GError          *error = NULL;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  context = g_option_context_new ("- Mutter-moblin devices panel");
  g_option_context_add_main_entries (context, _options, GETTEXT_PACKAGE);
  g_option_context_add_group (context, clutter_get_option_group_without_init ());
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_critical ("%s %s", G_STRLOC, error->message);
      g_critical ("Starting in standalone mode.");
      g_clear_error (&error);
      _standalone = TRUE;
    }
  g_option_context_free (context);

  MPL_PANEL_CLUTTER_INIT_WITH_GTK (&argc, &argv);

  mx_texture_cache_load_cache (mx_texture_cache_get_default (),
    DATADIR "/icons/moblin/48x48/mx.cache");
  mx_texture_cache_load_cache (mx_texture_cache_get_default (),
    DATADIR "/icons/hicolor/48x48/mx.cache");
  mx_texture_cache_load_cache (mx_texture_cache_get_default (),
    DATADIR "/mutter-moblin/mx.cache");

  mx_texture_cache_load_cache (mx_texture_cache_get_default (), MX_CACHE);
  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/panel.css", NULL);

  if (_standalone)
    {
      Window xwin;

      stage = clutter_stage_get_default ();
      clutter_actor_realize (stage);
      xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

      MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK_FOR_XID (xwin);

      content = create_panel (NULL);
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), content);

      g_signal_connect (stage, "notify::width",
                        G_CALLBACK (stage_width_notify_cb), content);
      g_signal_connect (stage, "notify::height",
                        G_CALLBACK (stage_height_notify_cb), content);

      clutter_actor_set_size (stage, 1024, 600);
      clutter_actor_show (stage);
      clutter_actor_grab_key_focus (content);

    } else {

      /* All button styling goes in mutter-moblin.css for now,
       * don't pass our own stylesheet. */
      MplPanelClient *panel = mpl_panel_clutter_new ("devices",
                                                     _("devices"),
                                                     /*THEMEDIR "/toolbar-button.css" */ NULL,
                                                     "devices-button",
                                                     TRUE);

      MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK (panel);

      stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (panel));
      content = create_panel (panel);
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), content);

      g_signal_connect (panel, "set-size",
                        G_CALLBACK (panel_set_size_cb), content);
    }

  /* Monitor buttons */
  notify_init ("Moblin Devices Panel");
  button_monitor = dalston_button_monitor_new ();

  clutter_main ();

  return EXIT_SUCCESS;
}

