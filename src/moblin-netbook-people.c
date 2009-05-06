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

#include <glib/gi18n.h>

#include <nbtk/nbtk.h>

#include "moblin-netbook-people.h"
#include "mnb-drop-down.h"
#include "moblin-netbook.h"
#include "mnb-entry.h"


#include <anerley/anerley-tp-feed.h>
#include <anerley/anerley-item.h>
#include <anerley/anerley-tile-view.h>
#include <anerley/anerley-tile.h>
#include <anerley/anerley-aggregate-tp-feed.h>



ClutterActor *
make_people_panel (MutterPlugin *plugin,
                   gint          width)
{
  NbtkWidget *vbox, *hbox, *label, *entry, *drop_down, *bin, *button;
  NbtkWidget *scroll_view;
  NbtkWidget *icon_view;
  ClutterText *text;
  guint items_list_width = 0, items_list_height = 0;
  MissionControl *mc;
  AnerleyFeed *feed;
  DBusGConnection *conn;
  ClutterModel *model;

  drop_down = mnb_drop_down_new ();

  vbox = nbtk_table_new ();
  clutter_actor_set_width (vbox, width);
  nbtk_table_set_col_spacing (NBTK_TABLE (vbox), 12);
  nbtk_table_set_row_spacing (NBTK_TABLE (vbox), 6);
  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), CLUTTER_ACTOR (vbox));
  clutter_actor_set_name (CLUTTER_ACTOR (vbox), "people-vbox");

  hbox = nbtk_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (hbox), "people-search");
  nbtk_table_set_col_spacing (NBTK_TABLE (hbox), 20);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (vbox),
                                        CLUTTER_ACTOR (hbox),
                                        0, 0,
                                        "row-span", 1,
                                        "col-span", 2,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        NULL);

  label = nbtk_label_new (_("People"));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "people-search-label");
  nbtk_table_add_actor_with_properties (NBTK_TABLE (hbox),
                                        CLUTTER_ACTOR (label),
                                        0, 0,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);

  entry = mnb_entry_new (_("Search"));
  clutter_actor_set_name (CLUTTER_ACTOR (entry), "people-search-entry");
  clutter_actor_set_width (CLUTTER_ACTOR (entry), 600);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (hbox),
                                        CLUTTER_ACTOR (entry),
                                        0, 1,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
  mc = mission_control_new (conn);
  feed = anerley_aggregate_tp_feed_new (mc);
  model = anerley_feed_model_new (feed);
  icon_view = anerley_tile_view_new (model);
  scroll_view = nbtk_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll_view),
                               (ClutterActor *)icon_view);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (vbox),
                                        (ClutterActor *)scroll_view,
                                        1,
                                        0,
                                        "x-fill",
                                        TRUE,
                                        "x-expand",
                                        TRUE,
                                        "y-expand",
                                        TRUE,
                                        "y-fill",
                                        TRUE,
                                        NULL);
  clutter_actor_show_all (vbox);

  return (ClutterActor *)drop_down;
}
