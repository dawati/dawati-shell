/* mwb-netpanel-scrollview.h */
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

#ifndef _MNB_NETPANEL_SCROLLVIEW_H
#define _MNB_NETPANEL_SCROLLVIEW_H

#include <glib-object.h>
#include <clutter/clutter.h>
#include <moblin-panel/mpl-entry.h>

G_BEGIN_DECLS

#define MNB_TYPE_NETPANEL_SCROLLVIEW mnb_netpanel_scrollview_get_type()

#define MNB_NETPANEL_SCROLLVIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MNB_TYPE_NETPANEL_SCROLLVIEW, MnbNetpanelScrollview))

#define MNB_NETPANEL_SCROLLVIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MNB_TYPE_NETPANEL_SCROLLVIEW, MnbNetpanelScrollviewClass))

#define MWB_IS_NETPANEL_SCROLLVIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MNB_TYPE_NETPANEL_SCROLLVIEW))

#define MWB_IS_NETPANEL_SCROLLVIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MNB_TYPE_NETPANEL_SCROLLVIEW))

#define MNB_NETPANEL_SCROLLVIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MNB_TYPE_NETPANEL_SCROLLVIEW, MnbNetpanelScrollviewClass))

typedef struct _MnbNetpanelScrollviewPrivate MnbNetpanelScrollviewPrivate;

typedef struct {
  NbtkWidget parent;
  
  MnbNetpanelScrollviewPrivate *priv;
} MnbNetpanelScrollview;

typedef struct {
  NbtkWidgetClass parent_class;

} MnbNetpanelScrollviewClass;

GType mnb_netpanel_scrollview_get_type (void);

NbtkWidget *mnb_netpanel_scrollview_new ();

void mnb_netpanel_scrollview_add_item (MnbNetpanelScrollview *self, guint order,
                                       ClutterActor *item, ClutterActor *title);

G_END_DECLS

#endif /* _MNB_NETPANEL_SCROLLVIEW_H */
