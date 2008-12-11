/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
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
 * Simple system tray test.
 */

#include "../src/moblin-netbook-system-tray.h"

static GtkWidget *message;
static GtkWidget *popup;

static void
activate_cb (GtkStatusIcon *status_icon, gpointer data)
{
  static int count = 0;

  g_message ("Received activate signal.");
  gtk_dialog_run (GTK_DIALOG (message));
  gtk_widget_hide (message);

  switch (++count)
    {
    case 1:
      gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (message),
				     "Come on! Was once not enough ?!");
      break;
    case 2:
      gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (message),
				     "You are pushing your luck, mate !!!");
    break;
    default:
      gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (message),
				     "That's it! I am calling your manager.");
    }
}

static void
popup_position_func (GtkMenu *menu, gint *x, gint *y,
		     gboolean *push, gpointer data)
{
  *x    = mnbk_event_x;
  *y    = mnbk_event_y;
  *push = FALSE;
}

static void
popup_menu_cb (GtkStatusIcon *icon, guint button, guint atime, gpointer data)
{
  g_message ("Received pop-up menu signal with coords %d, %d.",
	     mnbk_event_x, mnbk_event_y);

  gtk_menu_popup (GTK_MENU (popup),
		  NULL, NULL, popup_position_func, NULL,
		  button, atime);
}

int
main (int argc, char *argv[])
{
  GtkStatusIcon *icon;
  GtkWidget     *item;

  gtk_init (&argc, &argv);

  message  = gtk_message_dialog_new (NULL,
				     GTK_DIALOG_MODAL,
				     GTK_MESSAGE_INFO,
				     GTK_BUTTONS_OK,
                                     "Thank you for activating the applet.\n"
				     "Now go and do something more useful!");

  popup = gtk_menu_new ();

  item = gtk_menu_item_new_with_label ("Really?");
  gtk_widget_show (item);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup), item);

  item = gtk_menu_item_new_with_label ("Nothing better to do?");
  gtk_widget_show (item);
  gtk_menu_shell_append (GTK_MENU_SHELL (popup), item);

  icon = gtk_status_icon_new_from_stock (GTK_STOCK_INFO);

  g_signal_connect (icon, "activate", G_CALLBACK (activate_cb), NULL);
  g_signal_connect (icon, "popup-menu", G_CALLBACK (popup_menu_cb), NULL);

  gdk_add_client_message_filter (gdk_atom_intern (MOBLIN_SYSTEM_TRAY_EVENT,
						  FALSE),
				 mnbtk_client_message_handler, icon);

  gtk_status_icon_set_visible (icon, TRUE);

  gtk_main ();

  return 0;
}
