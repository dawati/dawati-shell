/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 * Derived from status panel, author: Emmanuele Bassi <ebassi@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MNB_PEOPLE_PANEL
#define _MNB_PEOPLE_PANEL

#include <glib-object.h>
#include <nbtk/nbtk.h>
#include "mnb-drop-down.h"

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
  NbtkTable parent;
} MnbPeoplePanel;

typedef struct {
  NbtkTableClass parent_class;
} MnbPeoplePanelClass;

GType mnb_people_panel_get_type (void);

NbtkWidget *mnb_people_panel_new (void);
void mnb_people_panel_set_dropdown (MnbPeoplePanel *people_panel,
                                    MnbDropDown    *drop_down);
G_END_DECLS

#endif /* _MNB_PEOPLE_PANEL */
