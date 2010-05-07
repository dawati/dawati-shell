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
#include "chrome/browser/pref_service.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/browser/browser_prefs.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/user_data_manager.h"

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

static gboolean
stage_delete_event (ClutterStage *stage,
                    ClutterEvent *event,
                    gpointer* userdata)
{
  MessageLoopForUI::current()->Quit();
  return TRUE;
}


static gboolean standalone = FALSE;

static GOptionEntry entries[] = {
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &standalone, "Do not embed into the mutter-moblin panel", NULL}
};

#define CHROME_EXE_PATH "/opt/google/chrome/"
#define CHROME_BUNDLE_PATH CHROME_EXE_PATH "/chrome.pak"
#define CHROME_LOCALE_PATH CHROME_EXE_PATH "/locales"

#define CHROMIUM_EXE_PATH "/usr/lib/chromium-browser/"
#define CHROMIUM_BUNDLE_PATH CHROMIUM_EXE_PATH "/chrome.pak"
#define CHROMIUM_LOCALE_PATH CHROMIUM_EXE_PATH "/locales"

int
main (int    argc,
      char **argv)
{
  MplPanelClient *client;
  ClutterActor *stage;
  MoblinNetbookNetpanel *netpanel;
  GOptionContext *context;
  std::string browser_name;
  GError *error = NULL;

  // Init chrome environment variables
  base::AtExitManager at_exit_manager;
  CommandLine::Init(argc, argv);
  CommandLine* cmd_line = CommandLine::ForCurrentProcess();

  chrome::RegisterPathProvider();
  app::RegisterPathProvider();

  if (g_file_test (CHROMIUM_BUNDLE_PATH, G_FILE_TEST_EXISTS)
     && g_file_test (CHROMIUM_LOCALE_PATH, G_FILE_TEST_EXISTS)) {
    FilePath bundle_path(CHROMIUM_BUNDLE_PATH);
    FilePath locale_path(CHROMIUM_LOCALE_PATH);
    ResourceBundle::InitSharedInstance(L"en-US", bundle_path, locale_path);
    browser_name = "google-chrome";
  }
  else {
    g_warning("Chrome browser is not installed\n");
  }

  g_thread_init(NULL);
  g_type_init();
  gtk_init(&argc, &argv);

  MessageLoop main_message_loop(MessageLoop::TYPE_UI);

  scoped_ptr<BrowserProcessImpl> browser_process;
  browser_process.reset(new BrowserProcessImpl(*cmd_line));

  ChromeThread main_thread(ChromeThread::UI, MessageLoop::current());

  PrefService* local_state = browser_process->local_state();
  local_state->RegisterStringPref(prefs::kApplicationLocale, L"");

  scoped_ptr<UserDataManager> user_data_manager(UserDataManager::Create());

  // Initialize the prefs of the local state.
  browser::RegisterLocalState(local_state);
  g_browser_process->SetApplicationLocale("en-US");

  // Create the child threads.  We need to do this since ChromeThread::PostTask
  // silently deletes a posted task if the target message loop isn't created.
  browser_process->db_thread();
  browser_process->file_thread();
  browser_process->process_launcher_thread();
  browser_process->io_thread();

  // XXX: have to init chrome profile prover after thread creation
  // Init chrome profile provider
  ChromeProfileProvider::GetInstance()->Initialize(browser_name.c_str());

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
    moblin_netbook_netpanel_set_browser(netpanel, browser_name.c_str());

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

    moblin_netbook_netpanel_set_panel_client (netpanel, client);
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

    MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK_FOR_XID (xwin);
    netpanel = MOBLIN_NETBOOK_NETPANEL (moblin_netbook_netpanel_new ());
    moblin_netbook_netpanel_set_browser(netpanel, browser_name.c_str());

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

  // run chromium's message loop
  MessageLoopForUI::current()->Run(NULL);

  return 0;
}
