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

#include <gtk/gtk.h>
#include <display.h>
#include <mutter-plugin.h>

#include "../moblin-netbook.h"
#include "mnb-notification-gtk.h"

static GtkWidget *notification = NULL;

void meta_window_unmake_fullscreen (MetaWindow  *window);

static gboolean
mnb_notification_gtk_click_cb (GtkWidget      *widget,
                               GdkEventButton *event,
                               gpointer        data)
{
  MutterPlugin *plugin  = moblin_netbook_get_plugin_singleton ();
  MetaScreen   *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay  *display = meta_screen_get_display (screen);
  MetaWindow   *mw = NULL;
  gboolean      fullscreen = FALSE;

  g_debug ("Gtk notification notifier clicked");

  g_object_get (display, "focus-window", &mw, NULL);

  if (!mw)
    {
      g_warning (G_STRLOC " Could not obtain currently focused window!");
      return TRUE;
    }

  g_object_get (mw, "fullscreen", &fullscreen, NULL);

  if (!fullscreen)
    {
      g_warning (G_STRLOC " Currently focused window is not fullscreen!");
      return TRUE;
    }

  meta_window_unmake_fullscreen (mw);

  return TRUE;
}

static GtkWidget *
mnb_notification_gtk_create ()
{
  GtkWidget *widget = NULL;
  GtkWindow *window;
  GtkWidget *button;

  widget = gtk_window_new (GTK_WINDOW_POPUP);
  window = GTK_WINDOW (widget);

  gtk_window_set_decorated (window, FALSE);
  gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_NOTIFICATION);
  gtk_window_set_resizable (window, FALSE);
  gtk_window_set_title (window, "mnb-notification-gtk");
  gtk_window_set_accept_focus (window, FALSE);

  gtk_window_move (window, 20, 20);

  button = gtk_button_new_with_label ("Test notification");
  gtk_widget_show (button);
  gtk_container_add (GTK_CONTAINER (window), button);

  g_signal_connect (button, "button-press-event",
                    G_CALLBACK(mnb_notification_gtk_click_cb),
                    NULL);
  return widget;
}


void
mnb_notification_gtk_show (void)
{
  if (G_UNLIKELY (notification == NULL))
    notification = mnb_notification_gtk_create ();

  if (notification)
    {
      gtk_widget_show (notification);
      gtk_window_present (GTK_WINDOW (notification));
    }
}

void
mnb_notification_gtk_hide (void)
{
  if (notification)
    gtk_widget_hide (notification);
}

