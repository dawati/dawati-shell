#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <dalston/dalston-volume-applet.h>
#include <dalston/dalston-power-applet.h>
#include "moblin-netbook-system-tray.h"

#include <config.h>

#define PADDING 4

static void
_plug_notify_embedded (GObject    *object,
                       GParamSpec *pspec,
                       gpointer    userdata)
{
  DalstonPowerApplet *applet = (DalstonPowerApplet *)userdata;
  gboolean embedded;

  g_object_get (object,
                "embedded",
                &embedded,
                NULL);

  dalston_power_applet_set_active (applet, embedded);
}

int
main (int    argc,
      char **argv)
{
  DalstonPowerApplet *power_applet;
  GtkStatusIcon *status_icon;
  GtkWidget *pane;
  GdkScreen *screen;
  GtkSettings *settings;
  GtkWidget *plug;

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  gtk_init (&argc, &argv);

  /* Force to the moblin theme */
  settings = gtk_settings_get_default ();
  gtk_settings_set_string_property (settings,
                                    "gtk-theme-name",
                                    "Moblin-Netbook",
                                    NULL);

  /* Power applet */
  power_applet = dalston_power_applet_new ();
  status_icon = dalston_power_applet_get_status_icon (power_applet);
  plug = gtk_plug_new (0);
  g_signal_connect (plug,
                    "notify::embedded",
                    (GCallback)_plug_notify_embedded,
                    power_applet);

  pane = dalston_power_applet_get_pane (power_applet);
  gtk_container_add (GTK_CONTAINER (plug),
                     pane);
  mnbk_system_tray_init (status_icon, GTK_PLUG (plug), "battery");
  screen = gtk_widget_get_screen (plug);
  gtk_widget_set_size_request (pane,
                               gdk_screen_get_width (screen) - 2 * PADDING,
                               -1);

  gtk_main ();
}
