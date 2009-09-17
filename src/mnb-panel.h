/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-panel.h */
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

#ifndef _MNB_PANEL
#define _MNB_PANEL

#include <glib-object.h>

#include "mnb-drop-down.h"

G_BEGIN_DECLS

#define MNB_TYPE_PANEL mnb_panel_get_type()

#define MNB_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_PANEL, MnbPanel))

#define MNB_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_PANEL, MnbPanelClass))

#define MNB_IS_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_PANEL))

#define MNB_IS_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_PANEL))

#define MNB_PANEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_PANEL, MnbPanelClass))

typedef struct _MnbPanelPrivate MnbPanelPrivate;

typedef struct {
  MnbDropDown parent;

  MnbPanelPrivate *priv;
} MnbPanel;

typedef struct {
  MnbDropDownClass parent_class;

  void (*ready)                 (MnbPanel *panel);
  void (*remote_process_died)   (MnbPanel *panel);
  void (*request_button_style)  (MnbPanel *panel, const gchar *style);
  void (*request_tooltip)       (MnbPanel *panel, const gchar *tooltip);
  void (*set_size)              (MnbPanel *panel, guint width, guint height);
} MnbPanelClass;

GType mnb_panel_get_type (void);

MnbPanel *mnb_panel_new (MutterPlugin *plugin,
                         const gchar  *dbus_name,
                         guint         width,
                         guint         height);

void mnb_panel_show_mutter_window (MnbPanel *panel, MutterWindow *mcw);

const gchar  *mnb_panel_get_name          (MnbPanel *panel);
const gchar  *mnb_panel_get_tooltip       (MnbPanel *panel);
const gchar  *mnb_panel_get_stylesheet    (MnbPanel *panel);
const gchar  *mnb_panel_get_button_style  (MnbPanel *panel);
guint         mnb_panel_get_xid           (MnbPanel *panel);
gboolean      mnb_panel_is_ready          (MnbPanel *panel);
void          mnb_panel_set_size          (MnbPanel *panel,
                                           guint     width,
                                           guint     height);
MutterWindow *mnb_panel_get_mutter_window (MnbPanel *panel);
void          mnb_panel_focus             (MnbPanel *panel);


G_END_DECLS

#endif /* _MNB_PANEL */

