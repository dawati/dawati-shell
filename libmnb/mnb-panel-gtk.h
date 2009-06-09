/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-panel-gtk.h */
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

#ifndef _MNB_PANEL_GTK
#define _MNB_PANEL_GTK

#include <gtk/gtk.h>

#include "mnb-panel-client.h"

G_BEGIN_DECLS

#define MNB_TYPE_PANEL_GTK mnb_panel_gtk_get_type()

#define MNB_PANEL_GTK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_PANEL_GTK, MnbPanelGtk))

#define MNB_PANEL_GTK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_PANEL_GTK, MnbPanelGtkClass))

#define MNB_IS_PANEL_GTK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_PANEL_GTK))

#define MNB_IS_PANEL_GTK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_PANEL_GTK))

#define MNB_PANEL_GTK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_PANEL_GTK, MnbPanelGtkClass))

typedef struct _MnbPanelGtkPrivate MnbPanelGtkPrivate;

typedef struct
{
  MnbPanelClient parent;

  MnbPanelGtkPrivate *priv;
} MnbPanelGtk;

typedef struct
{
  MnbPanelClientClass parent_class;

} MnbPanelGtkClass;

GType mnb_panel_gtk_get_type (void);

MnbPanelClient *mnb_panel_gtk_new   (const gchar *dbus_path,
                                     const gchar *name,
                                     const gchar *tooltip,
                                     const gchar *stylesheet,
                                     const gchar *button_style);

GtkWidget *mnb_panel_gtk_get_window (MnbPanelGtk *panel);

G_END_DECLS

#endif /* _MNB_PANEL_GTK */

