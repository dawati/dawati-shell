/* moblin-netbook-netpanel.h */
/*
 * Copyright (c) 2009 Intel Corp.
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

#ifndef _MOBLIN_NETBOOK_NETPANEL_H
#define _MOBLIN_NETBOOK_NETPANEL_H

#include <glib-object.h>
#include <nbtk/nbtk.h>
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
  NbtkWidget parent;
  
  MoblinNetbookNetpanelPrivate *priv;
} MoblinNetbookNetpanel;

typedef struct {
  NbtkWidgetClass parent_class;
} MoblinNetbookNetpanelClass;

GType moblin_netbook_netpanel_get_type (void);

NbtkWidget* moblin_netbook_netpanel_new (void);

void moblin_netbook_netpanel_focus (MoblinNetbookNetpanel *netpanel);
void moblin_netbook_netpanel_clear (MoblinNetbookNetpanel *netpanel);

void moblin_netbook_netpanel_set_panel_client (MoblinNetbookNetpanel *netpanel,
                                               MplPanelClient *panel_client);

G_END_DECLS

#endif /* _MOBLIN_NETBOOK_NETPANEL_H */
