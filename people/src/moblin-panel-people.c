#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <nbtk/nbtk.h>
#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
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
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &standalone, "Do not embed into the mutter-moblin panel", NULL}
};

int
main (int    argc,
      char **argv)
{
  MplPanelClient *client;
  ClutterActor *stage;
  NbtkWidget *people_panel;
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

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             THEMEDIR "/panel.css", NULL);

  if (!standalone)
  {
    client = mpl_panel_clutter_new (MPL_PANEL_PEOPLE,
                                    _("people"),
                                    NULL,
                                    "people-button",
                                    TRUE);

    MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK (client);

    mpl_panel_client_set_height_request (client, 400);

    stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));
    people_panel = mnb_people_panel_new ();
    mnb_people_panel_set_panel_client (MNB_PEOPLE_PANEL (people_panel), client);
    g_signal_connect (client,
                      "set-size",
                      (GCallback)_client_set_size_cb,
                      people_panel);
  } else {
    Window xwin;

    stage = clutter_stage_get_default ();
    clutter_actor_realize (stage);
    xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

    MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK_FOR_XID (xwin);
    people_panel = mnb_people_panel_new ();
    clutter_actor_set_size ((ClutterActor *)people_panel, 1016, 400);
    clutter_actor_set_size (stage, 1016, 400);
    clutter_actor_show_all (stage);
  }

  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               (ClutterActor *)people_panel);


  clutter_main ();

  return 0;
}
