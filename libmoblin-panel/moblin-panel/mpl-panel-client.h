/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mpl-panel-client.h */
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

#ifndef _MPL_PANEL_CLIENT
#define _MPL_PANEL_CLIENT

#include <glib-object.h>
#include <X11/X.h>

#include "mpl-panel-common.h"

G_BEGIN_DECLS

#define MPL_TYPE_PANEL_CLIENT mpl_panel_client_get_type()

#define MPL_PANEL_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPL_TYPE_PANEL_CLIENT, MplPanelClient))

#define MPL_PANEL_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPL_TYPE_PANEL_CLIENT, MplPanelClientClass))

#define MPL_IS_PANEL_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPL_TYPE_PANEL_CLIENT))

#define MPL_IS_PANEL_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPL_TYPE_PANEL_CLIENT))

#define MPL_PANEL_CLIENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPL_TYPE_PANEL_CLIENT, MplPanelClientClass))

typedef struct _MplPanelClientPrivate MplPanelClientPrivate;

typedef struct
{
  GObject parent;

  MplPanelClientPrivate *priv;
} MplPanelClient;

typedef struct
{
  GObjectClass parent_class;

  /*
   * Public signals -- connect to these from your panel.
   */
  gboolean (*unload)         (MplPanelClient *panel);
  void     (*set_size)       (MplPanelClient *panel, guint width, guint height);
  void     (*set_position)   (MplPanelClient *panel, gint x, gint y);
  void     (*show)           (MplPanelClient *panel);
  void     (*show_begin)     (MplPanelClient *panel);
  void     (*show_end)       (MplPanelClient *panel);
  void     (*hide)           (MplPanelClient *panel);
  void     (*hide_begin)     (MplPanelClient *panel);
  void     (*hide_end)       (MplPanelClient *panel);

  /*
   * Private signals
   * Signals for DBus -- these are the interface signals.
   */
  void (*request_focus)        (MplPanelClient *panel);
  void (*request_button_style) (MplPanelClient *panel, const gchar *style);
  void (*request_tooltip)      (MplPanelClient *panel, const gchar *tooltip);
  void (*request_button_state) (MplPanelClient *panel, MnbButtonState state);
  void (*request_modality)     (MplPanelClient *panel, gboolean modal);
} MplPanelClientClass;

GType mpl_panel_client_get_type (void);

MplPanelClient *mpl_panel_client_new       (guint        xid,
                                            const gchar *name,
                                            const gchar *tooltip,
                                            const gchar *stylesheet,
                                            const gchar *button_style);

void  mpl_panel_client_unload               (MplPanelClient *panel);

void  mpl_panel_client_set_height_request   (MplPanelClient *panel,
                                             guint           height);
guint mpl_panel_client_get_height_request   (MplPanelClient *panel);

void  mpl_panel_client_show                 (MplPanelClient *panel);
void  mpl_panel_client_hide                 (MplPanelClient *panel);
void  mpl_panel_client_request_focus        (MplPanelClient *panel);
void  mpl_panel_client_request_button_style (MplPanelClient *panel,
                                             const gchar    *style);
void  mpl_panel_client_request_tooltip      (MplPanelClient *panel,
                                             const gchar    *tooltip);
void  mpl_panel_client_request_button_state (MplPanelClient *panel,
                                             MnbButtonState  state);
void  mpl_panel_client_request_modality     (MplPanelClient *panel,
                                             gboolean        modal);

gboolean mpl_panel_client_launch_application   (MplPanelClient *panel,
                                                const gchar    *app);

gboolean
mpl_panel_client_launch_application_from_desktop_file (MplPanelClient *panel,
                                                       const gchar    *desktop,
                                                       GList          *files);

gboolean
mpl_panel_client_launch_default_application_for_uri (MplPanelClient *panel,
                                                     const gchar    *uri);

Window mpl_panel_client_get_xid (MplPanelClient *panel);

void mpl_panel_client_ready (MplPanelClient *panel);

G_END_DECLS

#endif /* _MPL_PANEL_CLIENT */

