/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Robert Staudinger <robertx.staudinger@intel.com>
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
#include <dawati-panel/mpl-app-launch-context.h>
#include <dawati-panel/mpl-panel-clutter.h>
#include <dawati-panel/mpl-panel-common.h>
#include "dawati-netbook-launcher.h"

#include "config.h"

static void
stage_width_notify_cb (ClutterActor  *stage,
                       GParamSpec    *pspec,
                       MnbLauncher   *launcher)
{
  guint width = clutter_actor_get_width (stage);

  mnb_launcher_clear_filter (launcher);
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
panel_set_size_cb (MplPanelClient *panel,
                   guint           width,
                   guint           height,
                   MnbLauncher    *launcher)
{
  mnb_launcher_clear_filter (launcher);
  clutter_actor_set_size (CLUTTER_ACTOR (launcher), width, height);
}

static void
launcher_activated_cb (MnbLauncher    *launcher,
                       const gchar    *desktop_file,
                       MplPanelClient *panel)
{
  mpl_panel_client_launch_application_from_desktop_file (panel,
                                                         desktop_file,
                                                         NULL);
  mpl_panel_client_hide (panel);
}

static void
commandline_launch_activated_cb (MnbLauncher    *launcher,
                                 const gchar    *commandline,
                                 MplPanelClient *panel)
{
  if (commandline)
    {
      if (mpl_panel_client_launch_application   (panel, commandline))
        mpl_panel_client_hide (panel);
    }
}

static void
standalone_launcher_activated_cb (MnbLauncher    *launcher,
                                  const gchar    *desktop_file,
                                  gpointer        user_data)
{
  GDesktopAppInfo *app_info;
  GError          *error = NULL;

  app_info = g_desktop_app_info_new_from_filename (desktop_file);

  g_app_info_launch (G_APP_INFO (app_info),
                     NULL,
                     mpl_app_launch_context_get_default (),
                     &error);
  if (error) {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  g_object_unref (app_info);
}

static void
panel_show_end_cb (MplPanelClient *panel,
                   MnbLauncher    *launcher)
{
  mx_focusable_accept_focus (MX_FOCUSABLE (launcher), MX_FOCUS_HINT_FIRST);
}

static void
panel_hide_end_cb (MplPanelClient *panel,
                   MnbLauncher    *launcher)
{
  mnb_launcher_clear_filter (launcher);
}

int
main (int     argc,
      char  **argv)
{
  static gboolean    _standalone = FALSE;
  static char const *_geometry = NULL;
  static int         _dpi = 0;
  static GOptionEntry _options[] = {
    { "standalone", 's', 0, G_OPTION_ARG_NONE, &_standalone,
      "Do not embed into the mutter-dawati panel", NULL },
    { "geometry", 'g', 0, G_OPTION_ARG_STRING, &_geometry,
      "Window geometry in standalone mode", NULL },
#if CLUTTER_CHECK_VERSION(1, 3, 0)
    { "clutter-font-dpi", 'd', 0, G_OPTION_ARG_INT, &_dpi,
      "Set clutter font resolution to <dpi>", "<dpi>" },
#endif
    { NULL }
  };

  ClutterActor    *stage;
  ClutterActor    *launcher;
  GOptionContext  *context;
  GError          *error = NULL;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  context = g_option_context_new ("- Mutter-dawati application launcher panel");
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

  mpl_panel_clutter_init_lib (&argc, &argv);

  if (_dpi)
    {
#if CLUTTER_CHECK_VERSION(1, 3, 0)
      ClutterSettings *settings = clutter_settings_get_default ();
      g_object_set (settings, "font-dpi", _dpi * 1000, NULL);
#endif
    }

  mx_texture_cache_load_cache (mx_texture_cache_get_default (),
    DATADIR "/icons/dawati/48x48/mx.cache");
  mx_texture_cache_load_cache (mx_texture_cache_get_default (),
    DATADIR "/icons/hicolor/48x48/mx.cache");
  mx_texture_cache_load_cache (mx_texture_cache_get_default (),
    DATADIR "/mutter-dawati/mx.cache");

  mx_texture_cache_load_cache (mx_texture_cache_get_default (), MX_CACHE);

  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/panel.css", NULL);


  if (_standalone)
    {
      stage = clutter_stage_new ();
      clutter_actor_realize (stage);

      if (_geometry)
        {
          int x, y;
          unsigned int width, height;
          XParseGeometry (_geometry, &x, &y, &width, &height);
          clutter_actor_set_size (stage, width, height);
        }
      else
        {
          clutter_actor_set_size (stage, 1024, 600);
        }

      launcher = mnb_launcher_new ();
      g_signal_connect (launcher, "launcher-activated",
                        G_CALLBACK (standalone_launcher_activated_cb), NULL);
      clutter_actor_add_child (stage, launcher);

      g_signal_connect (stage, "notify::width",
                        G_CALLBACK (stage_width_notify_cb), launcher);
      g_signal_connect (stage, "notify::height",
                        G_CALLBACK (stage_height_notify_cb), launcher);

      clutter_actor_show (stage);

    } else {

      MplPanelClient  *panel;

      /* All button styling goes in mutter-dawati.css for now,
       * don't pass our own stylesheet. */
      panel = mpl_panel_clutter_new ("applications",
                                      _("applications"),
                                     NULL,
                                     "applications-button",
                                     TRUE);

      stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (panel));

      mpl_panel_client_set_size_request (panel, -1, 600);

      launcher = mnb_launcher_new ();

      g_signal_connect (launcher, "launcher-activated",
                        G_CALLBACK (launcher_activated_cb), panel);
      g_signal_connect (launcher, "commandline-launch-activated",
                        G_CALLBACK (commandline_launch_activated_cb), panel);
      g_signal_connect (panel, "show-end",
                        G_CALLBACK (panel_show_end_cb), launcher);
      g_signal_connect (panel, "hide-end",
                        G_CALLBACK (panel_hide_end_cb), launcher);

      clutter_actor_add_child (stage, launcher);

      g_signal_connect (panel, "size-changed",
                        G_CALLBACK (panel_set_size_cb), launcher);
    }

  mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));

  /* The last item in the launcher is the filter entry */
  mx_focusable_accept_focus (MX_FOCUSABLE (launcher), MX_FOCUS_HINT_FIRST);

  clutter_main ();


  return EXIT_SUCCESS;
}

