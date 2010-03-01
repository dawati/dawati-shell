/* moblin-netbook-netpanel.h */
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

#ifndef _MOBLIN_NETBOOK_NETPANEL_H
#define _MOBLIN_NETBOOK_NETPANEL_H

#include <glib-object.h>
#include <mx/mx.h>
#include <moblin-panel/mpl-panel-clutter.h>

G_BEGIN_DECLS

#define MOBLIN_TYPE_NETBOOK_NETPANEL moblin_netbook_netpanel_get_type()

#define MOBLIN_NETBOOK_NETPANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MOBLIN_TYPE_NETBOOK_NETPANEL, MoblinNetbookNetpanel))

#define MOBLIN_NETBOOK_NETPANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MOBLIN_TYPE_NETBOOK_NETPANEL, MoblinNetbookNetpanelClass))

#define MOBLIN_IS_NETBOOK_NETPANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MOBLIN_TYPE_NETBOOK_NETPANEL))

#define MOBLIN_IS_NETBOOK_NETPANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MOBLIN_TYPE_NETBOOK_NETPANEL))

#define MOBLIN_NETBOOK_NETPANEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MOBLIN_TYPE_NETBOOK_NETPANEL, MoblinNetbookNetpanelClass))

typedef struct _MoblinNetbookNetpanelPrivate MoblinNetbookNetpanelPrivate;

typedef struct {
  MxWidget parent;
  
  MoblinNetbookNetpanelPrivate *priv;
} MoblinNetbookNetpanel;

typedef struct {
  MxWidgetClass parent_class;
} MoblinNetbookNetpanelClass;

GType moblin_netbook_netpanel_get_type (void);

MxWidget* moblin_netbook_netpanel_new (void);

void moblin_netbook_netpanel_focus (MoblinNetbookNetpanel *netpanel);
void moblin_netbook_netpanel_clear (MoblinNetbookNetpanel *netpanel);

void moblin_netbook_netpanel_set_panel_client (MoblinNetbookNetpanel *netpanel,
                                               MplPanelClient *panel_client);

void moblin_netbook_netpanel_button_press (MoblinNetbookNetpanel *netpanel);

G_END_DECLS

#endif /* _MOBLIN_NETBOOK_NETPANEL_H */
