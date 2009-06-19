#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <nbtk/nbtk.h>
#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include "mnb-people-panel.h"

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

int
main (int    argc,
      char **argv)
{
  MplPanelClient *client;
  ClutterActor *stage;
  NbtkWidget *people_panel;

  clutter_init (&argc, &argv);

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             MUTTER_MOBLIN_CSS, NULL);

  client = mpl_panel_clutter_new (MPL_PANEL_PEOPLE,
                                  _("people"),
                                  MUTTER_MOBLIN_CSS,
                                  "people-button",
                                  TRUE);

  mpl_panel_client_set_height_request (client, 400);

  stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));
  people_panel = mnb_people_panel_new ();
  mnb_people_panel_set_panel_client (MNB_PEOPLE_PANEL (people_panel), client);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               (ClutterActor *)people_panel);
  g_signal_connect (client,
                    "set-size",
                    (GCallback)_client_set_size_cb,
                    people_panel);

  clutter_main ();

  return 0;
}
