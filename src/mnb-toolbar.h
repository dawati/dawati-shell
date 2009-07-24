/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-toolbar.c */
/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Authors: Matthew Allum <matthew.allum@intel.com>
 *          Tomas Frydrych <tf@linux.intel.com>
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

#ifndef _MNB_TOOLBAR
#define _MNB_TOOLBAR

#include <glib-object.h>
#include <nbtk/nbtk.h>

#include "mnb-panel.h"
#include "moblin-netbook.h"
#include "moblin-netbook-tray-manager.h"

G_BEGIN_DECLS

#define MNB_TYPE_TOOLBAR mnb_toolbar_get_type()

#define MNB_TOOLBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_TOOLBAR, MnbToolbar))

#define MNB_TOOLBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_TOOLBAR, MnbToolbarClass))

#define MNB_IS_TOOLBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_TOOLBAR))

#define MNB_IS_TOOLBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_TOOLBAR))

#define MNB_TOOLBAR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_TOOLBAR, MnbToolbarClass))

typedef struct _MnbToolbarPrivate MnbToolbarPrivate;

typedef struct {
  NbtkBin parent;

  MnbToolbarPrivate *priv;
} MnbToolbar;

typedef struct {
  NbtkBinClass parent_class;

  void (*show_completed) (MnbToolbar *toolbar);
  void (*hide_begin)     (MnbToolbar *toolbar);
  void (*hide_completed) (MnbToolbar *toolbar);
} MnbToolbarClass;

GType mnb_toolbar_get_type (void);

NbtkWidget* mnb_toolbar_new (MutterPlugin *plugin);

gboolean mnb_toolbar_is_tray_config_window (MnbToolbar *toolbar, Window xwin);

void mnb_toolbar_append_panel_old (MnbToolbar  *toolbar,
                                   const gchar *name,
                                   const gchar *tooltip);

void mnb_toolbar_activate_panel (MnbToolbar *toolbar, const gchar *panel_name);
void mnb_toolbar_deactivate_panel (MnbToolbar *toolbar, const gchar *panel_name);

const gchar * mnb_toolbar_get_active_panel_name (MnbToolbar *toolbar);

gboolean mnb_toolbar_in_transition (MnbToolbar *toolbar);

NbtkWidget * mnb_toolbar_get_switcher (MnbToolbar *toolbar);

void mnb_toolbar_set_dont_autohide (MnbToolbar *toolbar, gboolean dont);

void mnb_toolbar_set_disabled (MnbToolbar *toolbar, gboolean disabled);

MnbPanel * mnb_toolbar_find_panel_for_xid (MnbToolbar *toolbar, guint xid);

void mnb_toolbar_append_tray_window (MnbToolbar *toolbar, MutterWindow *mcw);

NbtkWidget * mnb_toolbar_get_active_panel (MnbToolbar *toolbar);

void mnb_toolbar_set_panel_input_only (MnbToolbar *toolbar, gboolean whether);

G_END_DECLS

#endif /* _MNB_TOOLBAR */

