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
#include <math.h>

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
  GtkWidget *child;
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
  MplPanelGtkPrivate  *priv = MPL_PANEL_GTK (self)->priv;
  MplPanelClientClass *p_class;

  p_class = MPL_PANEL_CLIENT_CLASS (mpl_panel_gtk_parent_class);

  gtk_window_resize (GTK_WINDOW (priv->window), width, height);

  if (p_class->set_size)
    p_class->set_size (self, width, height);
}

static void
mpl_panel_gtk_set_position (MplPanelClient *self, gint x, gint y)
{
  MplPanelGtkPrivate  *priv = MPL_PANEL_GTK (self)->priv;
  MplPanelClientClass *p_class;

  p_class = MPL_PANEL_CLIENT_CLASS (mpl_panel_gtk_parent_class);

  gtk_window_move (GTK_WINDOW (priv->window), x, y);

  if (p_class->set_position)
    p_class->set_position (self, x, y);
}

static void
mpl_panel_gtk_show (MplPanelClient *self)
{
  MplPanelGtkPrivate *priv = MPL_PANEL_GTK (self)->priv;

  gtk_widget_show (priv->window);
}

static void
mpl_panel_gtk_hide (MplPanelClient *self)
{
  MplPanelGtkPrivate *priv = MPL_PANEL_GTK (self)->priv;

  gtk_widget_hide (priv->window);
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

  client_class->set_position     = mpl_panel_gtk_set_position;
  client_class->set_size         = mpl_panel_gtk_set_size;
  client_class->show             = mpl_panel_gtk_show;
  client_class->hide             = mpl_panel_gtk_hide;
}

static void
mpl_panel_gtk_init (MplPanelGtk *self)
{
  MplPanelGtkPrivate *priv;

  priv = self->priv = MPL_PANEL_GTK_GET_PRIVATE (self);
}

static void
mpl_panel_gtk_constructed (GObject *self)
{
  MplPanelGtkPrivate *priv = MPL_PANEL_GTK (self)->priv;
  GtkWidget *window;

  if (G_OBJECT_CLASS (mpl_panel_gtk_parent_class)->constructed)
    G_OBJECT_CLASS (mpl_panel_gtk_parent_class)->constructed (self);

  priv->window = window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DOCK);
  gtk_window_stick (GTK_WINDOW (window));

  /*
   * Realize the window so we can get the xid.
   */
  gtk_widget_realize (window);

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

void
mpl_panel_gtk_set_child (MplPanelGtk *panel, GtkWidget *child)
{
  MplPanelGtkPrivate *priv = MPL_PANEL_GTK (panel)->priv;

  if (priv->child)
    gtk_container_remove (GTK_CONTAINER (priv->window), priv->child);

  priv->child = child;

  gtk_container_add (GTK_CONTAINER (priv->window), child);
}

GtkWidget *
mpl_panel_gtk_get_window (MplPanelGtk *panel)
{
  MplPanelGtkPrivate *priv = MPL_PANEL_GTK (panel)->priv;

  return priv->window;
}

