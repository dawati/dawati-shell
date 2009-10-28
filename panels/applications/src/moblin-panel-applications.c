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
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <nbtk/nbtk.h>
#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
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
panel_set_size_cb (MplPanelClient *panel,
                   guint           width,
                   guint           height,
                   MnbLauncher    *launcher)
{
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
  mpl_panel_client_request_hide (panel);
}

static void
standalone_launcher_activated_cb (MnbLauncher    *launcher,
                                  const gchar    *desktop_file,
                                  gpointer        user_data)
{
  GDesktopAppInfo *app_info;
  GError          *error = NULL;

  app_info = g_desktop_app_info_new_from_filename (desktop_file);

  g_app_info_launch (G_APP_INFO (app_info), NULL, NULL, &error);
  if (error) {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  g_object_unref (app_info);
}

static void
panel_show_begin_cb (MplPanelClient *panel,
                     MnbLauncher    *launcher)
{
  mnb_launcher_ensure_filled (launcher);
}

static void
panel_show_end_cb (MplPanelClient *panel,
                   MnbLauncher    *launcher)
{
  clutter_actor_grab_key_focus (CLUTTER_ACTOR (launcher));
}

static void
panel_hide_end_cb (MplPanelClient *panel,
                   MnbLauncher    *launcher)
{
  mnb_launcher_clear_filter (launcher);
}

static gboolean _standalone = FALSE;

static GOptionEntry _options[] = {
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &_standalone, "Do not embed into the mutter-moblin panel", NULL},
  { NULL }
};

int
main (int     argc,
      char  **argv)
{
  ClutterActor    *stage;
  ClutterActor    *launcher;
  GOptionContext  *context;
  GError          *error = NULL;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  context = g_option_context_new ("- Mutter-moblin application launcher panel");
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

  nbtk_texture_cache_load_cache(nbtk_texture_cache_get_default(),
    DATADIR "/icons/moblin/48x48/nbtk.cache");
  nbtk_texture_cache_load_cache(nbtk_texture_cache_get_default(),
    DATADIR "/icons/hicolor/48x48/nbtk.cache");
  nbtk_texture_cache_load_cache(nbtk_texture_cache_get_default(),
    DATADIR "/mutter-moblin/nbtk.cache");

  nbtk_texture_cache_load_cache (nbtk_texture_cache_get_default (),
                                 NBTK_CACHE);
  nbtk_style_load_from_file (nbtk_style_get_default (),
                             THEMEDIR "/panel.css", NULL);

  if (_standalone)
    {
      Window xwin;

      stage = clutter_stage_get_default ();
      clutter_actor_realize (stage);
      xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

      MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK_FOR_XID (xwin);

      launcher = mnb_launcher_new ();
      g_signal_connect (launcher, "launcher-activated",
                        G_CALLBACK (standalone_launcher_activated_cb), NULL);
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), launcher);

      g_signal_connect (stage, "notify::width",
                        G_CALLBACK (stage_width_notify_cb), launcher);
      g_signal_connect (stage, "notify::height",
                        G_CALLBACK (stage_height_notify_cb), launcher);

      clutter_actor_set_size (stage, 1024, 600);
      clutter_actor_show (stage);

    } else {

      MplPanelClient  *panel;

      /* All button styling goes in mutter-moblin.css for now,
       * don't pass our own stylesheet. */
      panel = mpl_panel_clutter_new (MPL_PANEL_APPLICATIONS,
                                      _("applications"),
                                     /*THEMEDIR "/toolbar-button.css" */ NULL,
                                     "applications-button",
                                     TRUE);

      MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK (panel);

      launcher = mnb_launcher_new ();
      g_signal_connect (launcher, "launcher-activated",
                        G_CALLBACK (launcher_activated_cb), panel);

      g_signal_connect (panel, "show-begin",
                        G_CALLBACK (panel_show_begin_cb), launcher);
      g_signal_connect (panel, "show-end",
                        G_CALLBACK (panel_show_end_cb), launcher);
      g_signal_connect (panel, "hide-end",
                        G_CALLBACK (panel_hide_end_cb), launcher);

      stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (panel));
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), launcher);

      g_signal_connect (panel, "set-size",
                        G_CALLBACK (panel_set_size_cb), launcher);
    }

  clutter_main ();

  return EXIT_SUCCESS;
}

