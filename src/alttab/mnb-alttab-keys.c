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

#include "mnb-alttab-keys.h"
#include "mnb-alttab.h"

/*
 * Metacity key handler for default Metacity bindings we want disabled.
 *
 * (This is necessary for keybidings that are related to the Alt+Tab shortcut.
 * In metacity these all use the src/ui/tabpopup.c object, which we have
 * disabled, so we need to take over all of those.)
 */
static void
mnb_alttab_nop_key_handler (MetaDisplay    *display,
                            MetaScreen     *screen,
                            MetaWindow     *window,
                            XEvent         *event,
                            MetaKeyBinding *binding,
                            gpointer        data)
{
}

void
mnb_alttab_overlay_setup_metacity_keybindings (MnbAlttabOverlay *overlay)
{
  /*
   * Bunch of standard Mutter shortcuts that we alias to the Alt+Tab
   */
  meta_keybindings_set_custom_handler ("switch_windows",
                                       mnb_alttab_overlay_alt_tab_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("switch_windows_backward",
                                       mnb_alttab_overlay_alt_tab_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("switch_panels",
                                       mnb_alttab_overlay_alt_tab_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("switch_panels_backward",
                                       mnb_alttab_overlay_alt_tab_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("cycle_group",
                                       mnb_alttab_overlay_alt_tab_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("cycle_group_backward",
                                       mnb_alttab_overlay_alt_tab_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("cycle_windows",
                                       mnb_alttab_overlay_alt_tab_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("cycle_windows_backward",
                                       mnb_alttab_overlay_alt_tab_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("cycle_panels",
                                       mnb_alttab_overlay_alt_tab_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("cycle_panels_backward",
                                       mnb_alttab_overlay_alt_tab_key_handler,
                                       overlay, NULL);

  meta_keybindings_set_custom_handler ("tab_popup_select",
                                       mnb_alttab_overlay_alt_tab_select_handler,
                                       overlay, NULL);

  meta_keybindings_set_custom_handler ("tab_popup_cancel",
                                       mnb_alttab_overlay_alt_tab_cancel_handler,
                                       overlay, NULL);

  /*
   * Install NOP handler for shortcuts that are related to Alt+Tab that we
   * want disabled.
   */
  meta_keybindings_set_custom_handler ("switch_group",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("switch_group_backward",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("switch_group_backward",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
#if 0
  meta_keybindings_set_custom_handler ("switch_to_workspace_left",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("switch_to_workspace_right",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
#endif
  meta_keybindings_set_custom_handler ("switch_to_workspace_up",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("switch_to_workspace_down",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);

  /*
   * Disable various shortcuts other that are not compatible with meego UI
   * paradigm -- strictly speaking not alttab related, but for now here.
   */
  meta_keybindings_set_custom_handler ("activate_window_menu",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("show_desktop",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("toggle_maximized",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("maximize",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("maximize_vertically",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("maximize_horizontally",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("unmaximize",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("minimize",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("toggle_shadow",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
  meta_keybindings_set_custom_handler ("close",
                                       mnb_alttab_nop_key_handler,
                                       overlay, NULL);
}
