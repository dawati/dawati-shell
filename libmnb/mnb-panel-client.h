/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-panel-client.h */
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

#ifndef _MNB_PANEL_CLIENT
#define _MNB_PANEL_CLIENT

#include <glib-object.h>

G_BEGIN_DECLS

#define MNB_TYPE_PANEL_CLIENT mnb_panel_client_get_type()

#define MNB_PANEL_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_PANEL_CLIENT, MnbPanelClient))

#define MNB_PANEL_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_PANEL_CLIENT, MnbPanelClientClass))

#define MNB_IS_PANEL_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_PANEL_CLIENT))

#define MNB_IS_PANEL_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_PANEL_CLIENT))

#define MNB_PANEL_CLIENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_PANEL_CLIENT, MnbPanelClientClass))

typedef struct _MnbPanelClientPrivate MnbPanelClientPrivate;

typedef struct
{
  GObject parent;

  MnbPanelClientPrivate *priv;
} MnbPanelClient;

typedef struct
{
  GObjectClass parent_class;

  /*
   * Public signals -- connect to these from your panel.
   */
  void (*set_size)           (MnbPanelClient *panel, guint width, guint height);
  void (*show_begin)         (MnbPanelClient *panel);
  void (*show_end)           (MnbPanelClient *panel);
  void (*hide_begin)         (MnbPanelClient *panel);
  void (*hide_end)           (MnbPanelClient *panel);

  /*
   * Private signals
   * Signals for DBus -- these are the interface signals.
   */
  void (*request_show)         (MnbPanelClient *panel);
  void (*request_hide)         (MnbPanelClient *panel);
  void (*request_focus)        (MnbPanelClient *panel);
  void (*request_button_style) (MnbPanelClient *panel, const gchar *style);
} MnbPanelClientClass;

GType mnb_panel_client_get_type (void);

MnbPanelClient *mnb_panel_client_new       (const gchar *dbus_path,
                                            guint        xid,
                                            const gchar *name,
                                            const gchar *tooltip,
                                            const gchar *stylesheet,
                                            const gchar *button_style);

void mnb_panel_client_request_show         (MnbPanelClient *panel);
void mnb_panel_client_request_hide         (MnbPanelClient *panel);
void mnb_panel_client_request_focus        (MnbPanelClient *panel);
void mnb_panel_client_request_button_style (MnbPanelClient *panel,
                                            const gchar    *style);

gboolean mnb_panel_client_launch_application   (MnbPanelClient *panel,
                                                const gchar    *app,
                                                gint            workspace,
                                                gboolean        no_chooser);

gboolean
mnb_panel_client_launch_application_from_desktop_file (MnbPanelClient *panel,
                                                       const gchar    *desktop,
                                                       GList          *files,
                                                       gint            wspace,
                                                       gboolean        no_chooser);

gboolean
mnb_panel_client_launch_default_application_for_uri (MnbPanelClient *panel,
                                                     const gchar    *uri,
                                                     gint            workspace,
                                                     gboolean        no_chooser);


G_END_DECLS

#endif /* _MNB_PANEL_CLIENT */

