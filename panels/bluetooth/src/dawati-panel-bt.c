/*
 * Copyright (c) 2012 Intel Corp.
 *
 * Author: Jussi Kukkonen <jussi.kukkonen@intel.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <locale.h>
#include <glib/gi18n.h>
#include <libnotify/notify.h>
#include <mx/mx.h>

#include <dawati-panel/mpl-app-launch-context.h>
#include <dawati-panel/mpl-panel-clutter.h>
#include <dawati-panel/mpl-panel-common.h>

#include "config.h"
#include "dawati-bt-shell.h"


/* functions for standalone mode, to simulate what happens in panel... */
static void
stage_width_notify_cb (ClutterActor  *stage,
                       GParamSpec    *pspec,
                       ClutterActor  *shell)
{
  clutter_actor_set_width (shell, clutter_actor_get_width (stage));
}

int
main (int     argc,
      char  **argv)
{
  static gboolean      standalone = FALSE;
  ClutterActor        *stage, *shell;
  MplPanelClient      *client;
  GOptionContext      *context;
  GError              *error = NULL;
  static GOptionEntry  options[] = {
    { "standalone", 's', 0, G_OPTION_ARG_NONE, &standalone,
      "Do not embed into the mutter-dawati toolbar", NULL },
    { NULL }
  };

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  context = g_option_context_new ("- Mutter-dawati bluetooth panel");
  g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
  g_option_context_add_group (context,
                              clutter_get_option_group_without_init ());
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_critical (G_STRLOC ": Error parsing option: %s", error->message);
    g_clear_error (&error);
  }
  g_option_context_free (context);

  /* Application name for notifications */
  notify_init (_("Bluetooth"));
  mpl_panel_clutter_init_lib (&argc, &argv);
  mpl_panel_clutter_load_base_style ();
  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/panel.css", NULL);

  if (standalone) {
    stage = clutter_stage_new ();
    shell = dawati_bt_shell_new (NULL);
    g_signal_connect (stage, "notify::width",
                      G_CALLBACK (stage_width_notify_cb), shell);
    clutter_actor_set_size (stage, 325, 620);

    clutter_container_add_actor (CLUTTER_CONTAINER (stage), shell);

    clutter_actor_show (stage);
  } else {
    client = mpl_panel_clutter_new ("bluetooth",
                                    _("bluetooth"),
                                    THEMEDIR "/toolbar-button.css",
                                    "state-off",
                                    TRUE);
    stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));
    shell = dawati_bt_shell_new (client);
    clutter_actor_set_size (shell, 325, -1);
    mpl_panel_clutter_set_child (MPL_PANEL_CLUTTER (client), shell);
    mpl_panel_clutter_track_actor_height (MPL_PANEL_CLUTTER (client),
                                          shell);
    clutter_actor_show (shell);
  }

  mx_focus_manager_get_for_stage (CLUTTER_STAGE (stage));

  clutter_main ();

  return 0;
}
