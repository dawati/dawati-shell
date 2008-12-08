/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *         Thomas Wood <thomas@linux.intel.com>
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
#include "moblin-netbook-ui.h"

extern MutterPlugin mutter_plugin;
static inline MutterPlugin *
mutter_get_plugin ()
{
  return &mutter_plugin;
}

/*
 * Clone lifecycle house keeping.
 *
 * The workspace switcher and chooser hold clones of the window glx textures.
 * We need to ensure that when the master texture disappears, we remove the
 * clone as well. We do this via weak reference on the original object, and
 * so when the clone itself disappears, we need to remove the weak reference
 * from the master.
 */

void
switcher_origin_weak_notify (gpointer data, GObject *object)
{
  ClutterActor *clone = data;

  /*
   * The original MutterWindow destroyed; remove the weak reference the
   * we added to the clone referencing the original window, then
   * destroy the clone.
   */
  g_object_weak_unref (G_OBJECT (clone), switcher_clone_weak_notify, object);
  clutter_actor_destroy (clone);
}

void
switcher_clone_weak_notify (gpointer data, GObject *object)
{
  ClutterActor *origin = data;

  /*
   * Clone destroyed -- this function gets only called whent the clone
   * is destroyed while the original MutterWindow still exists, so remove
   * the weak reference we added on the origin for sake of the clone.
   */
  g_object_weak_unref (G_OBJECT (origin), switcher_origin_weak_notify, object);
}

/*
 * Creates an iconic representation of the workspace with the label provided.
 *
 * We use the custom NutterWsIcon actor, which automatically handles layout
 * when the icon is resized.
 */
ClutterActor *
make_workspace_label (const gchar *text)
{
  NutterWsIcon *icon;
  ClutterActor *actor;
  ClutterColor  b_clr = { 0x44, 0x44, 0x44, 0xff };
  ClutterColor  f_clr = { 0xff, 0xff, 0xff, 0xff };

  actor = nutter_ws_icon_new ();
  icon  = NUTTER_WS_ICON (actor);

  clutter_actor_set_size (actor, WORKSPACE_CELL_WIDTH, WORKSPACE_CELL_HEIGHT);

  nutter_ws_icon_set_font_name (icon, "Sans 16");
  nutter_ws_icon_set_text (icon, text);
  nutter_ws_icon_set_color (icon, &b_clr);
  nutter_ws_icon_set_border_width (icon, 3);
  nutter_ws_icon_set_text_color (icon, &f_clr);
  nutter_ws_icon_set_border_color (icon, &f_clr);

  return actor;
}

