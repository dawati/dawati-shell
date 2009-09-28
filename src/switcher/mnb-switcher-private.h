/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Moblin Netbook
 * Copyright © 2009, Intel Corporation.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
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

#ifndef _MNB_SWITCHER_PRIVATE
#define _MNB_SWITCHER_PRIVATE

struct _MnbSwitcherPrivate {
  MutterPlugin    *plugin;
  NbtkWidget      *table;
  MnbSwitcherZone *new_zone;
  NbtkTooltip     *active_tooltip;
  GList           *global_tab_list;
  GList           *last_workspaces;

  MnbSwitcherItem *selected_item;
  MnbSwitcherZone *selected_zone;

  guint            show_completed_id;
  guint            hide_panel_cb_id;

  gboolean         dnd_in_progress     : 1;
  gboolean         constructing        : 1;
  gboolean         in_alt_grab         : 1;
  gboolean         waiting_for_timeout : 1;
  gboolean         alt_tab_down        : 1;
  gboolean         empty               : 1;
};

void mnb_switcher_advance (MnbSwitcher *switcher, gboolean backward);
void mnb_switcher_activate_selection (MnbSwitcher *switcher,
                                      gboolean     close,
                                      guint        timestamp);

#endif /* _MNB_SWITCHER_PRIVATE */

