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

#ifndef _MNB_PANEL_OOP
#define _MNB_PANEL_OOP

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "mnb-drop-down.h"

G_BEGIN_DECLS

#define MNB_TYPE_PANEL_OOP mnb_panel_oop_get_type()

#define MNB_PANEL_OOP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_PANEL_OOP, MnbPanelOop))

#define MNB_PANEL_OOP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_PANEL_OOP, MnbPanelOopClass))

#define MNB_IS_PANEL_OOP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_PANEL_OOP))

#define MNB_IS_PANEL_OOP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_PANEL_OOP))

#define MNB_PANEL_OOP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_PANEL_OOP, MnbPanelOopClass))

typedef struct _MnbPanelOopPrivate MnbPanelOopPrivate;

typedef struct {
  GObject parent;

  MnbPanelOopPrivate *priv;
} MnbPanelOop;

typedef struct {
  GObjectClass parent_class;

  void (*ready)                 (MnbPanelOop *panel);
  void (*remote_process_died)   (MnbPanelOop *panel);
} MnbPanelOopClass;

GType mnb_panel_oop_get_type (void);

MnbPanelOop *mnb_panel_oop_new (const gchar  *dbus_name,
                                gint          x,
                                gint          y,
                                guint         width,
                                guint         height);

void mnb_panel_oop_show_mutter_window (MnbPanelOop *panel, MutterWindow *mcw);

const gchar  *mnb_panel_oop_get_dbus_name     (MnbPanelOop *panel);
guint         mnb_panel_oop_get_xid           (MnbPanelOop *panel);
gboolean      mnb_panel_oop_is_ready          (MnbPanelOop *panel);

MutterWindow *mnb_panel_oop_get_mutter_window (MnbPanelOop *panel);
void          mnb_panel_oop_focus             (MnbPanelOop *panel);
gboolean      mnb_panel_oop_owns_window       (MnbPanelOop *panel, MutterWindow *mcw);
gboolean      mnb_panel_oop_is_ancestor_of_transient (MnbPanelOop     *panel,
                                                      MutterWindow *mcw);

void          mnb_toolbar_ping_panel_oop      (DBusGConnection *dbus_conn,
                                               const gchar     *dbus_name);

void          mnb_panel_oop_hide_animate      (MnbPanelOop *panel);

G_END_DECLS

#endif /* _MNB_PANEL_OOP */

