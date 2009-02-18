/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Tomas Frydrych  <tf@linux.intel.com>
 *         Chris Lord      <christopher.lord@intel.com>
 *         Emmanuele Bassi <ebassi@linux.intel.com>
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

#include "moblin-netbook.h"
#include "moblin-netbook-status.h"
#include "mnb-drop-down.h"
#include <nbtk/nbtk.h>
#include <gtk/gtk.h>
#include <string.h>


#define ICON_SIZE       48
#define PADDING         8
#define BORDER_WIDTH    4

static const gchar *services[] = {
  "twitter"
};

static const guint n_services = G_N_ELEMENTS (services);

static void
add_status_entries (NbtkTable *table)
{
  gint i;

  for (i = 0; i < n_services; i++)
    nbtk_table_add_widget (table, mnb_status_row_new (services[i]), i + 1, 0);
}

ClutterActor *
make_status (MutterPlugin *plugin, gint width)
{
  ClutterActor  *stage, *table;
  NbtkWidget    *drop_down, *footer, *up_button;
  NbtkWidget    *header;

  table = CLUTTER_ACTOR (nbtk_table_new ());
  nbtk_widget_set_style_class_name (NBTK_WIDGET (table), "MnbStatusPageTable");
  clutter_actor_set_width (CLUTTER_ACTOR (table), width);
  clutter_actor_set_reactive (table, TRUE);

  header = nbtk_label_new ("Your current status");
  nbtk_widget_set_style_class_name (header, "MnbStatusPageHeader");
  nbtk_table_add_widget_full (NBTK_TABLE (table), header,
                              0, 0,
                              1, 1,
                              0,
                              0.0, 0.5);

  add_status_entries (NBTK_TABLE (table));

  drop_down = mnb_drop_down_new ();
  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), table);

  return CLUTTER_ACTOR (drop_down);
}

/* vim:set ts=8 expandtab */
