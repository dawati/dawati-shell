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

#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-panel-gtk.h>
#include <config.h>

#define PKG_THEMEDIR PKG_DATADIR"/theme"

int
main (int    argc,
      char **argv)
{
  DalstonVolumeApplet *volume_applet;
  GtkWidget *pane;
  GtkWidget *window;
  GtkSettings *settings;
  MplPanelClient *panel_client;

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_init (&argc, &argv);

  /* Force to the moblin theme */
  settings = gtk_settings_get_default ();
  gtk_settings_set_string_property (settings,
                                    "gtk-theme-name",
                                    "Moblin-Netbook",
                                    NULL);

  panel_client = mpl_panel_gtk_new (MPL_PANEL_VOLUME,
                                    _("sound"),
                                    PKG_THEMEDIR "/volume-applet.css",
                                    "unknown",
                                    TRUE);
  /* FIXME is this correct? */
  mpl_panel_client_set_height_request (panel_client, 200);

  /* Volume applet */
  volume_applet = dalston_volume_applet_new (panel_client);

  window = mpl_panel_gtk_get_window (MPL_PANEL_GTK (panel_client));
  pane = dalston_volume_applet_get_pane (volume_applet);
  gtk_container_add (GTK_CONTAINER (window), pane);
  gtk_widget_show_all (window);

  gtk_main ();
}
