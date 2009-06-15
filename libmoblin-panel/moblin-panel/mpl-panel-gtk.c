/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mpl-panel-gtk.c */
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

#include <gdk/gdkx.h>

#include "mpl-panel-gtk.h"

G_DEFINE_TYPE (MplPanelGtk, mpl_panel_gtk, MPL_TYPE_PANEL_CLIENT)

#define MPL_PANEL_GTK_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_PANEL_GTK, MplPanelGtkPrivate))

static void mpl_panel_gtk_constructed (GObject *self);

enum
{
  PROP_0,
};

struct _MplPanelGtkPrivate
{
  GtkWidget *window;
};

static void
mpl_panel_gtk_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_panel_gtk_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_panel_gtk_dispose (GObject *self)
{
  G_OBJECT_CLASS (mpl_panel_gtk_parent_class)->dispose (self);
}

static void
mpl_panel_gtk_finalize (GObject *object)
{
  G_OBJECT_CLASS (mpl_panel_gtk_parent_class)->finalize (object);
}

static void
mpl_panel_gtk_set_size (MplPanelClient *self, guint width, guint height)
{
  MplPanelGtkPrivate *priv   = MPL_PANEL_GTK (self)->priv;
  GtkWidget          *window = priv->window;

  g_debug ("Setting panel window size to %dx%d", width, height);

  gtk_widget_set_size_request (window, width, height);
}

static void
mpl_panel_gtk_class_init (MplPanelGtkClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  MplPanelClientClass *client_class = MPL_PANEL_CLIENT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MplPanelGtkPrivate));

  object_class->get_property     = mpl_panel_gtk_get_property;
  object_class->set_property     = mpl_panel_gtk_set_property;
  object_class->dispose          = mpl_panel_gtk_dispose;
  object_class->finalize         = mpl_panel_gtk_finalize;
  object_class->constructed      = mpl_panel_gtk_constructed;

  client_class->set_size = mpl_panel_gtk_set_size;
}

static void
mpl_panel_gtk_init (MplPanelGtk *self)
{
  MplPanelGtkPrivate *priv;

  priv = self->priv = MPL_PANEL_GTK_GET_PRIVATE (self);
}

static gboolean
mpl_panel_gtk_window_delete_event_cb (GtkWidget *window,
                                      GdkEvent  *event,
                                      gpointer   data)
{
  /*
   * Ensure the config window will be available.
   */
  g_debug ("delete-event: interupting");
  gtk_widget_hide (window);

  /*
   * Returning TRUE here stops the widget from getting destroyed.
   */
  return TRUE;
}

static void
mpl_panel_gtk_embedded_cb (GtkWidget *widget, GParamSpec *pspec, gpointer data)
{
  gboolean embedded;

  g_object_get (widget, "embedded", &embedded, NULL);

  if (embedded)
    gtk_widget_show (widget);
  else
    gtk_widget_hide (widget);
}

static void
mpl_panel_gtk_constructed (GObject *self)
{
  MplPanelGtkPrivate *priv = MPL_PANEL_GTK (self)->priv;
  GtkWidget *window;

  if (G_OBJECT_CLASS (mpl_panel_gtk_parent_class)->constructed)
    G_OBJECT_CLASS (mpl_panel_gtk_parent_class)->constructed (self);

  priv->window = window = gtk_plug_new (0);

  gtk_widget_realize (window);

  g_signal_connect (window, "delete-event",
                    G_CALLBACK (mpl_panel_gtk_window_delete_event_cb), self);

  g_signal_connect (window, "notify::embedded",
                    G_CALLBACK (mpl_panel_gtk_embedded_cb), self);

  g_object_set (self, "xid", GDK_WINDOW_XID (window->window), NULL);
}

MplPanelClient *
mpl_panel_gtk_new (const gchar *name,
                   const gchar *tooltip,
                   const gchar *stylesheet,
                   const gchar *button_style,
                   gboolean     with_toolbar_service)
{
  MplPanelClient *panel = g_object_new (MPL_TYPE_PANEL_GTK,
                                        "name",            name,
                                        "tooltip",         tooltip,
                                        "stylesheet",      stylesheet,
                                        "button-style",    button_style,
                                        "toolbar-service", with_toolbar_service,
                                        NULL);

  return panel;
}

GtkWidget *
mpl_panel_gtk_get_window (MplPanelGtk *panel)
{
  return panel->priv->window;
}

