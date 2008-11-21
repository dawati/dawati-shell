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

/*
 * This file contains some utility functions shared between the UI components.
 * TODO -- see if can get rid of most of this.
 */
extern MutterPlugin mutter_plugin;
static inline MutterPlugin *
mutter_get_plugin ()
{
  return &mutter_plugin;
}

/*
 * Returns a container for n-th workspace.
 *
 * list -- WS containers; if the n-th container does not exist, it is
 *         appended.
 *
 * active -- index of the currently active workspace (to be highlighted)
 *
 */
ClutterActor *
ensure_nth_workspace (GList **list, gint n, gint active)
{
  MutterPlugin  *plugin = mutter_get_plugin ();
  GList         *l      = *list;
  GList         *tmp    = NULL;
  gint           i      = 0;
  gint           screen_width, screen_height;

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  while (l)
    {
      if (i == n)
        return l->data;

      l = l->next;
      ++i;
    }

  g_assert (i <= n);

  while (i <= n)
    {
      ClutterActor *group;
      ClutterColor  background_clr = { 0, 0, 0, 0};
      ClutterColor  active_clr =     { 0xfd, 0xd9, 0x09, 0x7f};
      ClutterActor *background;


      /*
       * For non-expanded group, we use NutterScaleGroup container, which
       * allows us to apply scale to the workspace en mass.
       */
      group = nutter_scale_group_new ();

      /*
       * We need to add background, otherwise if the ws is empty, the group
       * will have size 0x0, and not respond to clicks.
       */
      if (i == active)
        background =  clutter_rectangle_new_with_color (&active_clr);
      else
        background =  clutter_rectangle_new_with_color (&background_clr);

      clutter_actor_set_size (background,
                              screen_width,
                              screen_height);

      clutter_container_add_actor (CLUTTER_CONTAINER (group), background);

      tmp = g_list_append (tmp, group);

      ++i;
    }

  g_assert (tmp);

  *list = g_list_concat (*list, tmp);

  return g_list_last (*list)->data;
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
