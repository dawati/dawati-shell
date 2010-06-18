/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
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

/*
 * Simple test for out of process panels.
 *
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <meego-panel/mpl-panel-gtk.h>
#include <meego-panel/mpl-panel-common.h>

/*
 * This is a callback to demonstrate how the application can close the config
 * window; however, you probably should not do that in your applicaiton, as
 * the hiding of the config window is taken care of by the Meego UI.
 */
static void
button_clicked_cb (GtkButton *button, gpointer data)
{
  MplPanelClient *panel = MPL_PANEL_CLIENT (data);

  mpl_panel_client_hide (panel);
}

/*
 * Function to create the content of the panel window.
 *
 * This is to demonstrate how the contents can be created dynamically when the
 * window is getting shown. If your window has static content, you can construct
 * it immediately after creating the panel object.
 */
static void
make_window_content (MplPanelGtk *panel)
{
  GtkWidget *table, *button, *entry, *toplevel;

  table = gtk_table_new (5, 3, TRUE);

  button = gtk_button_new_from_stock (GTK_STOCK_QUIT);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (button_clicked_cb), panel);

  gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 1, 2);

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), "test");
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 3, 4);

  gtk_widget_show_all (table);

  mpl_panel_gtk_set_child (panel, table);

  toplevel = gtk_widget_get_toplevel (table);

  if (GTK_WIDGET_TOPLEVEL (toplevel) && GTK_IS_WINDOW (toplevel))
    gtk_window_set_focus (GTK_WINDOW (toplevel), entry);
}

/*
 * Change the style of the panel button every time we are called.
 */
static gboolean
change_button_style_cb (gpointer data)
{
  static int count = 0;
  static int height = 520;

  MplPanelClient *panel = MPL_PANEL_CLIENT (data);

  count++;
  height -= 10;

  if (height < 100)
    height = 520;

  if (count % 2)
    mpl_panel_client_request_button_style (panel, "state2");
  else
    mpl_panel_client_request_button_style (panel, "state1");

#if 1
  mpl_panel_client_set_height_request (panel, height);
#endif
  return TRUE;
}

int
main (int argc, char *argv[])
{
  MplPanelClient *panel;

  gtk_init (&argc, &argv);

  /*
   * NB: the toolbar service indicates whether this panel requires access
   *     to the API provided by org.meego.Mpl.Toolbar -- if you need to do
   *     any application launching, etc., then pass TRUE.
   */
  panel = mpl_panel_gtk_new (MPL_PANEL_TEST,           /* the panel slot */
                             "test",                   /* tooltip */
                             CSS_DIR"/test-panel.css", /*stylesheet */
                             "state1",                 /* button style */
                             TRUE);                    /* no toolbar service*/

  /*
   * Strictly speaking, it is not necessary to construct the window contents
   * at this point, the panel can instead hook to MplPanelClient::set-size
   * signal. However, once the set-size signal has been emitted, the panel
   * window must remain in a state suitable to it being shown.
   *
   * The panel can also hook into the MplPanelClient::show-begin signal, to be
   * when it is being shown, but this signal is assynchronous, so that the
   * panel might finish showing *before* the Panel handles this signal.
   */
  make_window_content (MPL_PANEL_GTK (panel));

  g_timeout_add (5000, change_button_style_cb, panel);

  gtk_main ();

  return 0;
}
