#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <nbtk/nbtk.h>
#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include <penge/penge-grid-view.h>

#include <config.h>

static void
_client_set_size_cb (MplPanelClient *client,
                     guint           width,
                     guint           height,
                     gpointer        userdata)
{
  clutter_actor_set_size ((ClutterActor *)userdata,
                          width,
                          height);
}

static void
_grid_view_activated_cb (PengeGridView *grid_view,
                         gpointer       userdata)
{
  mpl_panel_client_request_hide ((MplPanelClient *)userdata);
}

int
main (int    argc,
      char **argv)
{
  MplPanelClient *client;
  ClutterActor *stage;
  NbtkWidget *grid_view;
  GOptionContext *context;
  GError *error = NULL;

  context = g_option_context_new ("- Mutter-moblin application myzone panel");
  g_option_context_add_group (context, clutter_get_option_group ());
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical (G_STRLOC ": Error parsing option: %s", error->message);
    g_clear_error (&error);
  }
  g_option_context_free (context);

  gtk_init (&argc, &argv);
  clutter_x11_set_display (GDK_DISPLAY ());
  clutter_init (&argc, &argv);

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             MUTTER_MOBLIN_CSS, NULL);

  client = mpl_panel_clutter_new (MPL_PANEL_MYZONE,
                                  _("myzone"),
                                  MUTTER_MOBLIN_CSS,
                                  "myzone-button",
                                  TRUE);

  stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));

  grid_view = g_object_new (PENGE_TYPE_GRID_VIEW,
                            "panel-client",
                            client,
                            NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               (ClutterActor *)grid_view);
  g_signal_connect (client,
                    "set-size",
                    (GCallback)_client_set_size_cb,
                    grid_view);

  g_signal_connect (grid_view,
                    "activated",
                    (GCallback)_grid_view_activated_cb,
                    client);

  clutter_main ();

  return 0;
}
