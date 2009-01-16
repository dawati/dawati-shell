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
#include "moblin-netbook-switcher.h"

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

void
toggle_control (MutterPlugin *plugin, MnbkControl control, gboolean show)
{
  MoblinNetbookPluginPrivate *priv  = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  ClutterActor               *actor = NULL;

  if (show)
    {
      if (control != MNBK_CONTROL_SPACES && priv->workspace_switcher)
        hide_workspace_switcher (plugin);

      if (control != MNBK_CONTROL_APPLICATIONS)
        clutter_actor_hide (priv->launcher);

      switch (control)
        {
        case MNBK_CONTROL_SPACES:
          actor = make_workspace_switcher (plugin);
          break;
        case MNBK_CONTROL_APPLICATIONS:
          break;
        default:
          break;
        }

      if (actor)
        {
          clutter_actor_set_position (actor, 4, PANEL_HEIGHT);
          clutter_actor_raise (actor, priv->panel_shadow);
          clutter_actor_show (actor);
        }
    }
  else
    {
      switch (control)
        {
        case MNBK_CONTROL_SPACES:
          hide_workspace_switcher (plugin);
          break;
        case MNBK_CONTROL_APPLICATIONS:
          break;
        default:
          break;
        }

      if (actor)
        clutter_actor_hide (actor);
    }
}


