/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-input-manager.h */
/*
 * Copyright (c) 2009 Intel Corp.
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

#ifndef MNB_INPUT_MANAGER
#define MNB_INPUT_MANAGER

#include <meta/meta-plugin.h>

typedef struct MnbInputRegion  MnbInputRegion;
typedef struct MnbInputManager MnbInputManager;

/*
 * Stack layers
 */
typedef enum
{
  MNB_INPUT_LAYER_PANEL = 0,         /* Where the Panel frames are      */
  MNB_INPUT_LAYER_PANEL_TRANSIENTS,  /* For any transients of Panels    */

  /* Must be last */
  MNB_INPUT_LAYER_TOP                /* Top layer (e.g., notifications) */
} MnbInputLayer;

void            mnb_input_manager_create  (MetaPlugin *plugin);
void            mnb_input_manager_destroy (void);
MnbInputRegion *mnb_input_manager_push_region (gint          x,
                                                gint          y,
                                                guint         width,
                                                guint         height,
                                                gboolean      inverse,
                                                MnbInputLayer layer);
void            mnb_input_manager_remove_region (MnbInputRegion  *mir);
void            mnb_input_manager_remove_region_without_update (MnbInputRegion  *mir);
void            mnb_input_manager_push_window (MetaWindowActor *mcw, MnbInputLayer layer);
void            mnb_input_manager_push_actor  (ClutterActor *actor, MnbInputLayer layer);
void            mnb_input_manager_push_oop_panel (MetaWindowActor *mcw);

#endif
