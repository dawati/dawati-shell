/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mpl-panel-gtk.h */
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

#ifndef _MPL_PANEL_GTK
#define _MPL_PANEL_GTK

#include <gtk/gtk.h>

#include "mpl-panel-client.h"

G_BEGIN_DECLS

#define MPL_PANEL_GTK_INIT MPL_PANEL_GTK_INIT__is_replaced_with__gtk_init

#define MPL_TYPE_PANEL_GTK mpl_panel_gtk_get_type()

#define MPL_PANEL_GTK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPL_TYPE_PANEL_GTK, MplPanelGtk))

#define MPL_PANEL_GTK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPL_TYPE_PANEL_GTK, MplPanelGtkClass))

#define MPL_IS_PANEL_GTK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPL_TYPE_PANEL_GTK))

#define MPL_IS_PANEL_GTK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPL_TYPE_PANEL_GTK))

#define MPL_PANEL_GTK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPL_TYPE_PANEL_GTK, MplPanelGtkClass))

typedef struct _MplPanelGtk        MplPanelGtk;
typedef struct _MplPanelGtkClass   MplPanelGtkClass;
typedef struct _MplPanelGtkPrivate MplPanelGtkPrivate;

/**
 * MplPanelGtk:
 *
 * Panel object for Gtk+-based panels.
 */
struct _MplPanelGtk
{
  /*<private>*/
  MplPanelClient parent;

  MplPanelGtkPrivate *priv;
};

/**
 * MplPanelGtkClass:
 *
 * Panel object for Gtk+-based panels.
 */
struct _MplPanelGtkClass
{
  /*<private>*/
  MplPanelClientClass parent_class;
};

GType mpl_panel_gtk_get_type (void);

MplPanelClient *mpl_panel_gtk_new   (const gchar *name,
                                     const gchar *tooltip,
                                     const gchar *stylesheet,
                                     const gchar *button_style,
                                     gboolean     with_toolbar_service);

GtkWidget *     mpl_panel_gtk_get_window (MplPanelGtk *panel);
void            mpl_panel_gtk_set_child (MplPanelGtk *panel, GtkWidget *child);

G_END_DECLS

#endif /* _MPL_PANEL_GTK */

