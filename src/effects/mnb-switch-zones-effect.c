/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Authors: Tomas Frydrych <tf@linux.intel.com>
 *          Chris Lord     <chris@linux.intel.com>
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

#include "mnb-switch-zones-effect.h"
#include "mnb-fancy-bin.h"
#include "mnb-zones-preview.h"

static ClutterActor *zones_preview = NULL;
static gint          running = 0;

static void
mnb_switch_zones_completed_cb (MnbZonesPreview *preview, MutterPlugin *plugin)
{
  clutter_actor_destroy (zones_preview);
  zones_preview = NULL;

  if (--running < 0)
    {
      g_warning (G_STRLOC ": error in running effect accounting!");
      running = 0;
    }

  mutter_plugin_switch_workspace_completed (plugin);
}

/*
 * This is the Metacity entry point for the effect.
 */
void
mnb_switch_zones_effect (MutterPlugin         *plugin,
                         gint                  from,
                         gint                  to,
                         MetaMotionDirection   direction)
{
  GList *w;
  gint width, height;
  MetaScreen *screen;
  ClutterActor *window_group;

  MeegoNetbookPluginPrivate *priv = MEEGO_NETBOOK_PLUGIN (plugin)->priv;

  if (running++)
    {
      /*
       * We have been called while the effect is already in progress; we need to
       * mutter know that we completed the previous run.
       */
      if (--running < 0)
        {
          g_warning (G_STRLOC ": error in running effect accounting!");
          running = 0;
        }

      mutter_plugin_switch_workspace_completed (plugin);
    }

  if ((from == to) && !zones_preview)
    {
      if (--running < 0)
        {
          g_warning (G_STRLOC ": error in running effect accounting!");
          running = 0;
        }

      mutter_plugin_switch_workspace_completed (plugin);

      return;
    }

  screen = mutter_plugin_get_screen (plugin);

  if (!zones_preview)
    {
      ClutterActor *stage;

      /* Construct the zones preview actor */
      zones_preview = mnb_zones_preview_new ();
      g_object_set (G_OBJECT (zones_preview),
                    "workspace", (gdouble)from,
                    NULL);

      /* Add it to the stage */
      stage = mutter_get_stage_for_screen (screen);
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), zones_preview);

      /* Attach to completed signal */
      g_signal_connect (zones_preview, "switch-completed",
                        G_CALLBACK (mnb_switch_zones_completed_cb), plugin);
    }

  mutter_plugin_query_screen_size (plugin, &width, &height);
  g_object_set (G_OBJECT (zones_preview),
                "workspace-width", (guint)width,
                "workspace-height", (guint)height,
                "workspace-bg", priv->desktop_tex,
                NULL);

  mnb_zones_preview_clear (MNB_ZONES_PREVIEW (zones_preview));
  mnb_zones_preview_set_n_workspaces (MNB_ZONES_PREVIEW (zones_preview),
                                      meta_screen_get_n_workspaces (screen));

  /* Add windows to zone preview actor */
  for (w = mutter_plugin_get_windows (plugin); w; w = w->next)
    {
      MutterWindow *window = w->data;
      gint workspace = mutter_window_get_workspace (window);
      MetaCompWindowType type = mutter_window_get_window_type (window);

      /*
       * Only show regular windows that are not sticky (getting stacking order
       * right for sticky windows would be really hard, and since they appear
       * on each workspace, they do not help in identifying which workspace
       * it is).
       */
      if ((workspace < 0) ||
          mutter_window_is_override_redirect (window) ||
          (type != META_COMP_WINDOW_NORMAL))
        continue;

      mnb_zones_preview_add_window (MNB_ZONES_PREVIEW (zones_preview), window);
    }

  /* Make sure it's on top */
  window_group = mutter_plugin_get_window_group (plugin);
  clutter_actor_raise (zones_preview, window_group);

  /* Initiate animation */
  mnb_zones_preview_change_workspace (MNB_ZONES_PREVIEW (zones_preview), to);
}

