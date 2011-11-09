/* dawati-netbook-netpanel.h */
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

#ifndef _DAWATI_NETBOOK_NETPANEL_H
#define _DAWATI_NETBOOK_NETPANEL_H

#include <glib-object.h>
#include <mx/mx.h>
#include <dawati-panel/mpl-panel-clutter.h>

G_BEGIN_DECLS

#define DAWATI_TYPE_NETBOOK_NETPANEL dawati_netbook_netpanel_get_type()

#define DAWATI_NETBOOK_NETPANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  DAWATI_TYPE_NETBOOK_NETPANEL, DawatiNetbookNetpanel))

#define DAWATI_NETBOOK_NETPANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  DAWATI_TYPE_NETBOOK_NETPANEL, DawatiNetbookNetpanelClass))

#define DAWATI_IS_NETBOOK_NETPANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  DAWATI_TYPE_NETBOOK_NETPANEL))

#define DAWATI_IS_NETBOOK_NETPANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  DAWATI_TYPE_NETBOOK_NETPANEL))

#define DAWATI_NETBOOK_NETPANEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  DAWATI_TYPE_NETBOOK_NETPANEL, DawatiNetbookNetpanelClass))

typedef struct _DawatiNetbookNetpanelPrivate DawatiNetbookNetpanelPrivate;

typedef struct {
  MxWidget parent;

  DawatiNetbookNetpanelPrivate *priv;
} DawatiNetbookNetpanel;

typedef struct {
  MxWidgetClass parent_class;
} DawatiNetbookNetpanelClass;

GType dawati_netbook_netpanel_get_type (void);

MxWidget* dawati_netbook_netpanel_new (void);

void dawati_netbook_netpanel_focus (DawatiNetbookNetpanel *netpanel);
void dawati_netbook_netpanel_clear (DawatiNetbookNetpanel *netpanel);

void dawati_netbook_netpanel_set_panel_client (DawatiNetbookNetpanel *netpanel,
                                               MplPanelClient *panel_client);

void dawati_netbook_netpanel_button_press (DawatiNetbookNetpanel *netpanel);

G_END_DECLS

#endif /* _DAWATI_NETBOOK_NETPANEL_H */
