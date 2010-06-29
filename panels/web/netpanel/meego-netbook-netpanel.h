/* meego-netbook-netpanel.h */
/*
 * Copyright (c) 2009 Intel Corp.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA. 
 */

#ifndef _MEEGO_NETBOOK_NETPANEL_H
#define _MEEGO_NETBOOK_NETPANEL_H

#include <glib-object.h>
#include <mx/mx.h>
#include <meego-panel/mpl-panel-clutter.h>

G_BEGIN_DECLS

#define MEEGO_TYPE_NETBOOK_NETPANEL meego_netbook_netpanel_get_type()

#define MEEGO_NETBOOK_NETPANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MEEGO_TYPE_NETBOOK_NETPANEL, MeegoNetbookNetpanel))

#define MEEGO_NETBOOK_NETPANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MEEGO_TYPE_NETBOOK_NETPANEL, MeegoNetbookNetpanelClass))

#define MEEGO_IS_NETBOOK_NETPANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MEEGO_TYPE_NETBOOK_NETPANEL))

#define MEEGO_IS_NETBOOK_NETPANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MEEGO_TYPE_NETBOOK_NETPANEL))

#define MEEGO_NETBOOK_NETPANEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MEEGO_TYPE_NETBOOK_NETPANEL, MeegoNetbookNetpanelClass))

typedef struct _MeegoNetbookNetpanelPrivate MeegoNetbookNetpanelPrivate;

typedef struct {
  MxWidget parent;
  
  MeegoNetbookNetpanelPrivate *priv;
} MeegoNetbookNetpanel;

typedef struct {
  MxWidgetClass parent_class;
} MeegoNetbookNetpanelClass;

GType meego_netbook_netpanel_get_type (void);

MxWidget* meego_netbook_netpanel_new (void);

void meego_netbook_netpanel_focus (MeegoNetbookNetpanel *netpanel);
void meego_netbook_netpanel_clear (MeegoNetbookNetpanel *netpanel);

void meego_netbook_netpanel_set_panel_client (MeegoNetbookNetpanel *netpanel,
                                               MplPanelClient *panel_client);

void meego_netbook_netpanel_button_press (MeegoNetbookNetpanel *netpanel);

void meego_netbook_netpanel_set_browser(MeegoNetbookNetpanel *netpanel,
                                         const char* browser_name);
G_END_DECLS

#endif /* _MEEGO_NETBOOK_NETPANEL_H */
