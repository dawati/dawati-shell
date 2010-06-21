/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 * Derived from status panel, author: Emmanuele Bassi <ebassi@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _MNB_PEOPLE_PANEL
#define _MNB_PEOPLE_PANEL

#include <glib-object.h>
#include <mx/mx.h>
#include <meego-panel/mpl-panel-clutter.h>
#include <meego-panel/mpl-panel-common.h>

G_BEGIN_DECLS

#define MNB_TYPE_PEOPLE_PANEL mnb_people_panel_get_type()

#define MNB_PEOPLE_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_PEOPLE_PANEL, MnbPeoplePanel))

#define MNB_PEOPLE_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_PEOPLE_PANEL, MnbPeoplePanelClass))

#define MNB_IS_PEOPLE_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_PEOPLE_PANEL))

#define MNB_IS_PEOPLE_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_PEOPLE_PANEL))

#define MNB_PEOPLE_PANEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_PEOPLE_PANEL, MnbPeoplePanelClass))

typedef struct {
  MxTable parent;
} MnbPeoplePanel;

typedef struct {
  MxTableClass parent_class;
} MnbPeoplePanelClass;

GType mnb_people_panel_get_type (void);

ClutterActor *mnb_people_panel_new (void);
void mnb_people_panel_set_panel_client (MnbPeoplePanel *people_panel,
                                        MplPanelClient *panel_client);
G_END_DECLS

#endif /* _MNB_PEOPLE_PANEL */
