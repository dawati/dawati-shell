/*
 * Dalston - power and volume applets for Moblin
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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
 *
 */


#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <dalston/dalston-volume-applet.h>
#include <dalston/dalston-volume-status-icon.h>
#include "moblin-netbook-system-tray.h"

#include <config.h>

#define PADDING 4

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

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
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
