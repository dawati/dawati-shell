#include <gtk/gtk.h>

#include <carrick/carrick-applet.h>
#include <carrick/carrick-status-icon.h>
#include "moblin-netbook-system-tray.h"

static void
_plug_notify_embedded (GObject    *object,
                       GParamSpec *pspec,
                       gpointer    userdata)
{
  CarrickStatusIcon *icon = CARRICK_STATUS_ICON (userdata);
  gboolean embedded;

  g_object_get (object,
                "embedded",
                &embedded,
                NULL);

  carrick_status_icon_set_active (icon, embedded);
}

int
main (int    argc,
      char **argv)
{
  CarrickApplet *applet;
  GtkWidget     *icon;
  GtkWidget     *pane;
  GdkScreen     *screen;
  GtkWidget     *plug;

  gtk_init (&argc, &argv);

  applet = carrick_applet_new ();
  icon = carrick_applet_get_icon (applet);
  plug = gtk_plug_new (0);
  g_signal_connect (plug,
                    "notify::embedded",
                    G_CALLBACK (_plug_notify_embedded),
                    icon);

  pane = carrick_applet_get_pane (applet);
  gtk_container_add (GTK_CONTAINER (plug),
                     pane);
  mnbk_system_tray_init (icon, GTK_PLUG (plug), "wifi");
  screen = gtk_widget_get_screen (plug);
  gtk_widget_set_size_request (pane,
                               gdk_screen_get_width (screen) - 2,
                               -1);

  gtk_main ();
}
