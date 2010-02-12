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

#include <locale.h>
#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <mx/mx.h>
#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include "moblin-netbook-netpanel.h"
#include "chrome-profile-provider.h"

#include <config.h>

// chrome header
#include "app/app_paths.h"
#include "app/resource_bundle.h"
#include "base/at_exit.h"
#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/i18n/icu_util.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/scoped_vector.h"
#include "base/string_util.h"
#include "base/command_line.h"
#include "chrome/common/pref_service.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/browser/browser_prefs.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_impl.h"

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
                          MoblinNetbookNetpanel *netpanel)
{
  moblin_netbook_netpanel_button_press (netpanel);
  return TRUE;
}

static gboolean standalone = FALSE;

static GOptionEntry entries[] = {
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &standalone, "Do not embed into the mutter-moblin panel", NULL}
};

#define CHROME_EXE_PATH "/usr/lib/chromium-browser"
#define CHROME_BUNDLE_PATH CHROME_EXE_PATH
#define CHROME_LOCALE_PATH CHROME_EXE_PATH "/locales"

int
main (int    argc,
      char **argv)
{
  MplPanelClient *client;
  ClutterActor *stage;
  MoblinNetbookNetpanel *netpanel;
  GOptionContext *context;
  GError *error = NULL;

  // Init chrome environment variables
  base::AtExitManager at_exit_manager;
  CommandLine::Init(argc, argv);
  CommandLine* cmd_line = CommandLine::ForCurrentProcess();

  chrome::RegisterPathProvider();
  app::RegisterPathProvider();

  FilePath bundle_path(CHROME_BUNDLE_PATH);
  FilePath locale_path(CHROME_LOCALE_PATH);
  ResourceBundle::InitSharedInstance(L"en-US", locale_path);

  scoped_ptr<BrowserProcessImpl> browser_process;
  browser_process.reset(new BrowserProcessImpl(*cmd_line));
  PrefService* local_state = browser_process->local_state();
  local_state->RegisterStringPref(prefs::kApplicationLocale, L"");

  // Initialize the prefs of the local state.
  browser::RegisterLocalState(local_state);
  g_browser_process->set_application_locale("en-US");

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

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


    MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK (client);

    stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));
    netpanel = MOBLIN_NETBOOK_NETPANEL (moblin_netbook_netpanel_new ());
    clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                                 CLUTTER_ACTOR (netpanel));
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

  g_signal_connect (stage,
                    "button-press-event",
                    (GCallback)stage_button_press_event,
                    netpanel);

  clutter_main ();

  return 0;
}
