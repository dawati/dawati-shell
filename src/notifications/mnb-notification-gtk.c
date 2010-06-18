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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <display.h>
#include <mutter-plugin.h>

#include "../meego-netbook.h"
#include "mnb-notification-gtk.h"

static GtkWidget *notification = NULL;
static GtkWidget *img_normal   = NULL;
static GtkWidget *img_hover    = NULL;

void meta_window_unmake_fullscreen (MetaWindow  *window);

static gboolean
mnb_notification_gtk_click_cb (GtkWidget      *widget,
                               GdkEventButton *event,
                               gpointer        data)
{
  MutterPlugin *plugin  = meego_netbook_get_plugin_singleton ();
  MetaScreen   *screen  = mutter_plugin_get_screen (plugin);
  MetaDisplay  *display = meta_screen_get_display (screen);
  MetaWindow   *mw = NULL;
  gboolean      fullscreen = FALSE;

  g_debug ("Got click on widget %s", G_OBJECT_TYPE_NAME (widget));

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

static gboolean
mnb_notification_gtk_crossing_cb (GtkWidget        *widget,
                                  GdkEventCrossing *event,
                                  gpointer          data)
{
  if (event->type == GDK_ENTER_NOTIFY)
    {
      if (gtk_widget_get_parent (img_normal) == widget)
        gtk_container_remove (GTK_CONTAINER (widget), img_normal);

      if (gtk_widget_get_parent (img_hover) != widget)
        gtk_container_add (GTK_CONTAINER (widget), img_hover);
      gtk_widget_show (img_hover);
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      if (gtk_widget_get_parent (img_hover) == widget)
        gtk_container_remove (GTK_CONTAINER (widget), img_hover);

      if (gtk_widget_get_parent (img_normal) != widget)
        gtk_container_add (GTK_CONTAINER (widget), img_normal);

      gtk_widget_show (img_normal);
    }

  return FALSE;
}


static GtkWidget *
mnb_notification_gtk_create ()
{
  GtkWidget *widget = NULL;
  GtkWindow *window;
  GtkWidget *evbox;
  GdkPixbuf *pixbuf;

  img_normal = gtk_image_new_from_file (THEMEDIR
                                        "/notifiers/fscreen-notif-normal.png");

  if (img_normal)
    g_object_ref (img_normal);

  img_hover  = gtk_image_new_from_file (THEMEDIR
                                        "/notifiers/fscreen-notif-hover.png");

  if (img_hover)
    g_object_ref (img_hover);

  widget = gtk_window_new (GTK_WINDOW_POPUP);
  window = GTK_WINDOW (widget);

  pixbuf = gdk_pixbuf_new_from_file (THEMEDIR
                                     "/notifiers/fscreen-notif-normal.png",
                                     NULL);

  if (pixbuf)
    {
      gint width, height, rowstride, channels;
      guchar *pixels;
      gint x, y;

      GdkDrawable *mask;
      GdkGC *gc;

      rowstride = gdk_pixbuf_get_rowstride (pixbuf);
      pixels    = gdk_pixbuf_get_pixels (pixbuf);
      width     = gdk_pixbuf_get_width (pixbuf);
      height    = gdk_pixbuf_get_height (pixbuf);
      channels = gdk_pixbuf_get_n_channels (pixbuf);

      g_assert (channels == 4);

      mask = gdk_pixmap_new (NULL, width, height, 1);
      gc   = gdk_gc_new (mask);

      for (x = 0; x < width; ++x)
        for (y = 0; y < height; ++y)
          {
            GdkColor clr;
            guchar *p = pixels + y * rowstride + x * channels;

            if (p[3] == 0)
              clr.pixel = 0;
            else
              clr.pixel = 1;

            gdk_gc_set_foreground (gc, &clr);

            gdk_draw_point (mask, gc, x, y);
          }

      gtk_widget_shape_combine_mask (widget, mask, 0, 0);

      g_object_unref (mask);
      g_object_unref (pixbuf);
      g_object_unref (gc);
    }

  gtk_window_set_decorated (window, FALSE);
  gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_NOTIFICATION);
  gtk_window_set_resizable (window, FALSE);
  gtk_window_set_title (window, "mnb-notification-gtk");
  gtk_window_set_accept_focus (window, FALSE);

  gtk_window_move (window, 20, 20);

  evbox = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (evbox), FALSE);
  gtk_event_box_set_above_child (GTK_EVENT_BOX (evbox), TRUE);

  if (!img_normal)
    {
      gtk_container_add (GTK_CONTAINER (evbox),
                         gtk_button_new_with_label ("Notifications"));
    }
  else
    {
      gtk_container_add (GTK_CONTAINER (evbox), img_normal);
    }

  gtk_container_add (GTK_CONTAINER (window), evbox);
  gtk_widget_show (evbox);

  g_signal_connect (evbox, "button-press-event",
                    G_CALLBACK(mnb_notification_gtk_click_cb),
                    NULL);

  if (img_normal && img_hover)
    {
      g_signal_connect (evbox, "enter-notify-event",
                        G_CALLBACK(mnb_notification_gtk_crossing_cb),
                        NULL);

      g_signal_connect (evbox, "leave-notify-event",
                        G_CALLBACK(mnb_notification_gtk_crossing_cb),
                        NULL);
    }

  return widget;
}

void
mnb_notification_gtk_show (void)
{
  if (G_UNLIKELY (notification == NULL))
    notification = mnb_notification_gtk_create ();

  if (notification)
    {
      MutterPlugin *plugin = meego_netbook_get_plugin_singleton ();
      gint          width, height, screen_width, screen_height;
      GtkWindow    *window = GTK_WINDOW (notification);

      mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);
      gtk_window_get_size (window, &width, &height);

      gtk_window_move (window,
                       screen_width - width - 2,
                       screen_height - height - 2);

      gtk_widget_show_all (notification);
      gtk_window_present (GTK_WINDOW (notification));
    }
}

void
mnb_notification_gtk_hide (void)
{
  if (notification)
    gtk_widget_hide (notification);
}

