#include <gtk/gtk.h>

#include <dalston/dalston-volume-applet.h>
#include <dalston/dalston-volume-status-icon.h>
#include "moblin-netbook-system-tray.h"

#define PADDING 0


static void
_plug_notify_embedded (GObject    *object,
                       GParamSpec *pspec,
                       gpointer    userdata)
{
  DalstonVolumeStatusIcon *status_icon = (DalstonVolumeStatusIcon *)userdata;
  gboolean embedded;

  g_object_get (object,
                "embedded",
                &embedded,
                NULL);

  dalston_volume_status_icon_set_active (status_icon, embedded);
}


int
main (int    argc,
      char **argv)
{
  DalstonVolumeApplet *volume_applet;
  GtkStatusIcon *status_icon;
  GtkWidget *pane;
  GdkScreen *screen;
  GtkSettings *settings;
  GtkWidget *plug;

  gtk_init (&argc, &argv);

  /* Force to the moblin theme */
  settings = gtk_settings_get_default ();
  gtk_settings_set_string_property (settings,
                                    "gtk-theme-name",
                                    "Moblin-Netbook",
                                    NULL);

  /* Volume applet */
  volume_applet = dalston_volume_applet_new ();
  status_icon = dalston_volume_applet_get_status_icon (volume_applet);
  plug = gtk_plug_new (0);
  g_signal_connect (plug,
                    "notify::embedded",
                    (GCallback)_plug_notify_embedded,
                    status_icon);

  pane = dalston_volume_applet_get_pane (volume_applet);
  gtk_container_add (GTK_CONTAINER (plug),
                     pane);
  mnbk_system_tray_init (status_icon, GTK_PLUG (plug), "sound");
  screen = gtk_widget_get_screen (plug);
  gtk_widget_set_size_request (pane,
                               gdk_screen_get_width (screen) - 2 * PADDING,
                               -1);

  gtk_main ();
}
