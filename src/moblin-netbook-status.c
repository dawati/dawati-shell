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
    nbtk_table_add_widget (table, mnb_status_entry_new (services[i]), i, 0);
}

ClutterActor *
make_status (MutterPlugin *plugin, gint width)
{
  ClutterActor  *stage, *table;
  gint           row, col, n_cols, pad;
  NbtkWidget    *drop_down, *footer, *up_button;
  NbtkWidget    *label;
  NbtkPadding    padding = { CLUTTER_UNITS_FROM_INT (4),
                             CLUTTER_UNITS_FROM_INT (4),
                             CLUTTER_UNITS_FROM_INT (4),
                             CLUTTER_UNITS_FROM_INT (4)};

  n_cols = (width - 2 * BORDER_WIDTH) / (ICON_SIZE + PADDING);

  /* Distribute any leftover space into the padding, if possible */
  pad = n_cols * (ICON_SIZE + PADDING) - (width - 2 * BORDER_WIDTH);

  if (pad >= n_cols)
    pad /= n_cols;
  else
    pad = 0;

  pad += PADDING;

  table = CLUTTER_ACTOR (nbtk_table_new ());
  nbtk_widget_set_padding (NBTK_WIDGET (table), &padding);

  clutter_actor_set_name (table, "status-table");
  clutter_actor_set_reactive (table, TRUE);

  nbtk_table_set_col_spacing (NBTK_TABLE (table), pad);
  nbtk_table_set_row_spacing (NBTK_TABLE (table), pad);

  drop_down = mnb_drop_down_new ();

  mnb_drop_down_set_child (MNB_DROP_DOWN (drop_down), table);

  clutter_actor_set_width (CLUTTER_ACTOR (table), width);

  nbtk_table_add_widget_full (NBTK_TABLE (table),
                              nbtk_label_new ("Your current status"),
                              0, 0,
                              1, 1,
                              NBTK_X_EXPAND | NBTK_X_FILL,
                              0.0, 0.5);

  add_status_entries (NBTK_TABLE (table));

  return CLUTTER_ACTOR (drop_down);
}

/* vim:set ts=8 expandtab */
