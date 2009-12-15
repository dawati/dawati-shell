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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <mx/mx.h>

#include "moblin-netbook-people.h"
#include "mnb-drop-down.h"
#include "moblin-netbook.h"
#include "mnb-entry.h"
#include "mnb-people-panel.h"


#include <anerley/anerley-tp-feed.h>
#include <anerley/anerley-item.h>
#include <anerley/anerley-tile-view.h>
#include <anerley/anerley-tile.h>
#include <anerley/anerley-aggregate-tp-feed.h>

ClutterActor *
make_people_panel (MutterPlugin *plugin,
                   gint          width)
{
  MxWidget *drop_down;
  MxWidget *people_panel;

  drop_down = (MxWidget *)mnb_drop_down_new (plugin);
  people_panel = mnb_people_panel_new ();
  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), (ClutterActor *)people_panel);
  mnb_people_panel_set_dropdown (MNB_PEOPLE_PANEL (people_panel), MNB_DROP_DOWN (drop_down));

  clutter_actor_set_size ((ClutterActor *)people_panel, width, 400);
  return (ClutterActor *)drop_down;
}
