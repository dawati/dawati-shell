/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Meego Netbook
 * Copyright © 2009, 2010, Intel Corporation.
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

#ifndef _MNB_ALTTAB_OVERLAY_PRIVATE
#define _MNB_ALTTAB_OVERLAY_PRIVATE

#include "mnb-alttab-overlay-app.h"

#define AUTOSCROLL_TRIGGER_TIMEOUT  750
#define AUTOSCROLL_ADVANCE_TIMEOUT  500

struct _MnbAlttabOverlayPrivate
{
  MnbAlttabOverlayApp *active;
  ClutterActor        *grid;

  gfloat               viewport_height;
  gfloat               scroll_y;
  guint                current_row;

  gboolean disposed            : 1;
  gboolean in_alt_grab         : 1;
  gboolean alt_tab_down        : 1;
  gboolean waiting_for_timeout : 1;
  gboolean backward            : 1;

  guint32 autoscroll_trigger_id;
  guint32 autoscroll_advance_id;
  guint32 slowdown_timeout_id;
};

void mnb_alttab_overlay_advance (MnbAlttabOverlay *overlay,
                                 gboolean            backward);
void mnb_alttab_overlay_activate_selection (MnbAlttabOverlay *overlay,
                                            guint               timestamp);

GList *mnb_alttab_overlay_get_app_list (MnbAlttabOverlay *overlay);

gboolean mnb_alttab_overlay_tab_still_down (MnbAlttabOverlay *overlay);

gboolean mnb_alttab_overlay_establish_keyboard_grab (MnbAlttabOverlay  *overlay,
                                                     MetaDisplay  *display,
                                                     MetaScreen   *screen,
                                                     gulong        mask,
                                                     guint         timestamp);

#endif
