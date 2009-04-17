/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

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

#include "mnb-switch-zones-effect.h"

#define MNBZE_ZOOM_IN_DURATION  100
#define MNBZE_ZOOM_OUT_DURATION 100
#define MNBZE_MOTION_DURATION   200
#define MNBZE_PAD 40

/*
 * The three stages of the effect
 */
typedef enum
{
  MNBZE_ZOOM_OUT = 0,
  MNBZE_MOTION,
  MNBZE_ZOOM_IN,
} MnbzeStage;

static ClutterActor     *frame        = NULL;
static ClutterActor     *strip        = NULL;
static ClutterActor     *desktop      = NULL;
static ClutterAnimation *zoom_anim    = NULL;
static ClutterAnimation *move_anim    = NULL;
static MutterWindow     *actor_for_cb = NULL;
static MnbzeStage        estage       = MNBZE_ZOOM_OUT;
static gint              current_to   = 0;
static gint              current_from = 0;

static void fill_strip (MutterPlugin*, NbtkTable*, gint, gint,
                        gdouble*, gdouble*);

/*
 * Release the ClutterClone actors we use for the effect.
 */
static void
release_clones (ClutterActor *clone, gpointer data)
{
  clutter_actor_destroy (clone);
}

/*
 * Data for our "completed" callbacks.
 */
struct effect_data
{
  MutterPlugin *plugin;
  gint          screen_width;
  gint          screen_height;
  guint         strip_cb_id;
};

static void
on_strip_animation_completed (ClutterAnimation *anim, gpointer data)
{
  struct effect_data         *edata  = data;

  switch (estage)
    {
    case MNBZE_ZOOM_IN:
    case MNBZE_ZOOM_OUT:
    default:
      g_warning ("Strip animation completion should not happen at this point.");
      break;

    case MNBZE_MOTION:
      {
        /*
         * The first part of the effect completed; start the second part.
         */
        estage = MNBZE_ZOOM_IN;

        /*
         * Since we are reusing this animation, we have to explicitely
         * disconnect this handler.
         */
        g_signal_handler_disconnect (anim, edata->strip_cb_id);
        edata->strip_cb_id = 0;

        g_slice_free (struct effect_data, data);

        clutter_actor_animate (frame, CLUTTER_LINEAR,
                               MNBZE_ZOOM_IN_DURATION,
                               "scale-x", 1.0,
                               "scale-y", 1.0,
                               NULL);
      }
    }
}

static void
on_frame_animation_completed (ClutterAnimation *anim, gpointer data)
{
  struct effect_data         *edata  = data;
  MutterPlugin               *plugin = edata->plugin;

  switch (estage)
    {
    case MNBZE_ZOOM_IN:
      {
        /*
         * The third stage of the effect is complete; hide the frame, release
         * the clones and tell Mutter we are done.
         */
        estage = MNBZE_ZOOM_OUT;

        zoom_anim = NULL;

        clutter_actor_hide (desktop);
        clutter_container_foreach (CLUTTER_CONTAINER (strip),
                                   release_clones, NULL);

        mutter_plugin_effect_completed (plugin, actor_for_cb,
                                        MUTTER_PLUGIN_SWITCH_WORKSPACE);

        g_slice_free (struct effect_data, data);
      }
      break;
    case MNBZE_ZOOM_OUT:
      {
        /*
         * The first part of the effect completed; start the second part.
         */
        struct effect_data *strip_data = g_slice_new0 (struct effect_data);
        gint screen_width              = edata->screen_width;
        ClutterAnimation *a;

        strip_data->plugin = plugin;
        strip_data->screen_width = edata->screen_width;
        strip_data->screen_height = edata->screen_height;

        estage = MNBZE_MOTION;

        a = clutter_actor_animate (strip, CLUTTER_LINEAR,
                                   MNBZE_MOTION_DURATION,
                                   "x",
                                   -MNBZE_PAD
                                   -((screen_width + 2*MNBZE_PAD)*current_to),
                                   NULL);

        /*
         * We want to reuse this animation, so add reference.
         */
        g_object_ref (a);

        strip_data->strip_cb_id =
          g_signal_connect (a, "completed",
                            G_CALLBACK (on_strip_animation_completed),
                            strip_data);

        if (move_anim != a)
          move_anim = a;
      }
      break;

    default:;
    }
}

/*
 * This is the Metacity entry point for the effect.
 *
 * Implementation details:
 *
 * The effect is split into three stages: zoom out, slide and zome in.
 *
 * Unlike the old effect, we do not mess about with the actual MutterWindows;
 * this way even if something goes wrong with the effect, once the effect
 * finishes the desktop is not messed up and remains usable.
 *
 * The effect is constructed of there layers of actors:
 *
 * desktop: this is the top level container for the effect; a NbtkBin with the
 *          *#zone-switch-background style.
 *
 * frame:   this is a window onto the desktop; this actor has the zoom effects
 *          applied to (it needs to be separate from the desktop, so that as we
 *          zoom the desktop background remains full screen).
 *
 * strip:   this an NbtkTable with a single row that contains the individual
 *          desktop thumbnails. Its position within the frame determines what
 *          what user sees; the strip has the motion part of the effect applied
 *          to it.
 */
void
mnb_switch_zones_effect (MutterPlugin         *plugin,
                         const GList         **actors,
                         gint                  from,
                         gint                  to,
                         MetaMotionDirection   direction)
{
  ClutterAnimation           *a;
  gdouble                     target_scale_x, target_scale_y;
  gint                        screen_width, screen_height;

  if (from == to)
    {
      mutter_plugin_effect_completed (plugin, NULL,
				      MUTTER_PLUGIN_SWITCH_WORKSPACE);
      return;
    }

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  if (G_UNLIKELY (!frame))
    {
      ClutterActor *parent;
      ClutterActor *bkg;

      desktop = nbtk_bin_new ();
      clutter_actor_set_name (desktop, "zone-switch-background");
      clutter_actor_set_size (desktop, screen_width, screen_height);

      strip = CLUTTER_ACTOR (nbtk_table_new ());

      clutter_actor_set_name (strip, "zone-switch-strip");

      nbtk_table_set_col_spacing (NBTK_TABLE (strip), MNBZE_PAD);

      frame = clutter_group_new ();

      /*
       * set up the frame so that any zooming is done around its center.
       */
      g_object_set (frame,
                    "scale-center-x", screen_width / 2,
                    "scale-center-y", screen_height / 2,
                    NULL);

      clutter_container_add_actor (CLUTTER_CONTAINER (frame), strip);

      clutter_container_add_actor (CLUTTER_CONTAINER (desktop), frame);

      parent = mutter_plugin_get_window_group (plugin);
      clutter_container_add_actor (CLUTTER_CONTAINER (parent), desktop);
      clutter_actor_hide (desktop);
    }

  /*
   * Make sure that the the whole effect is above all the windows.
   */
  clutter_actor_raise_top (desktop);

  /*
   * Construct the strip.
   */
  fill_strip (plugin, NBTK_TABLE (strip),
              screen_width, screen_height,
              &target_scale_x, &target_scale_y);

  /*
   * Position the strip so that the current desktop is in the frame window.
   */
  clutter_actor_set_position (strip,
                              -MNBZE_PAD - (screen_width + 2*MNBZE_PAD) * from,
                              0);

  clutter_actor_show (desktop);

  /*
   * Start the zoom out effect.
   */
  a = clutter_actor_animate (frame, CLUTTER_LINEAR,
                             MNBZE_ZOOM_OUT_DURATION,
                             "scale-x", target_scale_x,
                             "scale-y", target_scale_y,
                             NULL);

  actor_for_cb = (*actors)->data;

  /*
   * The reference management of the animation in clutter is somewhat peculiar.
   * Each time the animation completes, a single reference is released from the
   * animation object. For our purposes, we want to run the same animation
   * twice, restarting it from the "completed" signal; for that we have to add
   * an extra reference to the animation object.
   *
   * If this function is called while the effect is in progress, we need to
   * restart.
   */
  if (zoom_anim != a)
    {
      /*
       * Brand new animation, set up the callback.
       */
      struct effect_data *data = g_slice_new0 (struct effect_data);

      current_to          = to;
      current_from        = from;
      data->plugin        = plugin;
      data->screen_width  = screen_width;
      data->screen_height = screen_height;

      /*
       * New animation object, take extra reference and connect to signals
       * for completion.
       */
      zoom_anim = g_object_ref (a);

      g_signal_connect (a, "completed",
                        G_CALLBACK (on_frame_animation_completed),
                        data);
    }
  else
    {
      /*
       * The animation was restarted, make appropriate changes.
       */
      current_to = to;
      current_from = from;

      switch (estage)
        {
        default:
          break;

        case MNBZE_ZOOM_IN:
          /*
           * We need new reference for the zoom_anim, reset state and restart
           * all
           */
          g_object_ref (a);
          estage = MNBZE_ZOOM_OUT;

          clutter_actor_animate (frame, CLUTTER_LINEAR,
                                 MNBZE_ZOOM_OUT_DURATION,
                                 "scale-x", target_scale_x,
                                 "scale-y", target_scale_y,
                                 NULL);
          break;
        case MNBZE_ZOOM_OUT:
          /*
           * This should just work since the motion magic is still to come.
           */
          break;
        case MNBZE_MOTION:
          /*
           * Change the running motion interaction. We also need extra reference
           * on the zoom animation to make sure it keeps alive.
           */
          g_object_ref (a);
          clutter_actor_animate (strip, CLUTTER_LINEAR,
                                 MNBZE_MOTION_DURATION,
                                 "x",
                                 -MNBZE_PAD
                                 -((screen_width + 2*MNBZE_PAD)*current_to),
                                 NULL);
          break;
        }
    }

  estage = MNBZE_ZOOM_OUT;
}

/*
 * Constructs the desktop thumbnails. At present these are just ClutterGroups,
 * we will probably want to wrap these in NbtkBin so they can be themed.
 */
static ClutterActor *
make_nth_workspace (MutterPlugin  *plugin,
                    GList        **list,
                    gint           n,
                    gint           screen_width,
                    gint           screen_height)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  GList *l   = *list;
  GList *tmp =  NULL;
  gint   i   =  0;

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
      ClutterActor *bkg;

      group = clutter_group_new ();

      bkg = clutter_clone_new (priv->parallax_tex);
      clutter_actor_set_size (bkg, screen_width, screen_height);
      clutter_container_add_actor (CLUTTER_CONTAINER (group), bkg);

      clutter_actor_set_clip (group, 0, 0, screen_width, screen_height);

      tmp = g_list_append (tmp, group);

      ++i;
    }

  g_assert (tmp);

  *list = g_list_concat (*list, tmp);

  return g_list_last (*list)->data;
}

static void clone_weak_notify (gpointer data, GObject *object);

/*
 * Handle an original MutterWindow going away ...
 */
static void
origin_weak_notify (gpointer data, GObject *object)
{
  ClutterActor *clone = data;

  /*
   * The original MutterWindow destroyed; remove the weak reference the
   * we added to the clone referencing the original window, then
   * destroy the clone.
   */
  g_object_weak_unref (G_OBJECT (clone), clone_weak_notify, object);
  clutter_actor_destroy (clone);
}

/*
 * Handle the clone destruction.
 */
static void
clone_weak_notify (gpointer data, GObject *object)
{
  ClutterActor *origin = data;

  /*
   * Clone destroyed -- this function gets only called whent the clone
   * is destroyed while the original MutterWindow still exists, so remove
   * the weak reference we added on the origin for sake of the clone.
   */
  g_object_weak_unref (G_OBJECT (origin), origin_weak_notify, object);
}

/*
 * Fills the 'film strip' frame with individual workspaces, and calculates the
 * target scale of the zoom out effect.
 */
static void
fill_strip (MutterPlugin *plugin,
            NbtkTable    *strip,
            gint          screen_width,
            gint          screen_height,
            gdouble      *target_scale_x,
            gdouble      *target_scale_y)
{
  GList        *l;
  gint          cell_width, cell_height;
  GList        *workspaces = NULL;
  gint          ws_count = 0;
  MetaScreen   *screen = mutter_plugin_get_screen (plugin);
  gint          n_workspaces;
  guint         w, h;

  n_workspaces = meta_screen_get_n_workspaces (screen);

  /*
   * Calculate the target scale factor so that when the whole strip is zoomed
   * out, all the workspaces will be visible.
   */
  cell_width = (screen_width -
                n_workspaces * 2 * MNBZE_PAD) / n_workspaces;

  cell_height = screen_height * cell_width / screen_width;

  *target_scale_x = (gdouble) (cell_width)/ (gdouble)screen_width;
  *target_scale_y = (gdouble) (cell_height) /(gdouble)screen_height;

  l = mutter_plugin_get_windows (plugin);

  while (l)
    {
      MutterWindow       *mw = l->data;
      MetaCompWindowType  type;
      MetaRectangle       rect;
      gint                ws_indx;
      ClutterActor       *texture;
      ClutterActor       *clone;
      ClutterActor       *workspace = NULL;

      type = mutter_window_get_window_type (mw);
      ws_indx = mutter_window_get_workspace (mw);

      /*
       * Only show regular windows that are not sticky (getting stacking order
       * right for sticky windows would be really hard, and since they appear
       * on each workspace, they do not help in identifying which workspace
       * it is).
       */
      if (ws_indx < 0                             ||
          mutter_window_is_override_redirect (mw) ||
          type != META_COMP_WINDOW_NORMAL)
        {
          l = l->next;
          continue;
        }

      texture = mutter_window_get_texture (mw);
      clone   = clutter_clone_new (texture);

      clutter_actor_set_reactive (clone, FALSE);

      g_object_weak_ref (G_OBJECT (mw), origin_weak_notify, clone);
      g_object_weak_ref (G_OBJECT (clone), clone_weak_notify, mw);

      workspace = make_nth_workspace (plugin, &workspaces, ws_indx,
                                      screen_width, screen_height);
      g_assert (workspace);

      meta_window_get_outer_rect (mutter_window_get_meta_window (mw), &rect);
      clutter_actor_set_position (clone, rect.x, rect.y);

      clutter_container_add_actor (CLUTTER_CONTAINER (workspace), clone);

      l = l->next;
    }

  l = workspaces;
  while (l)
    {
      ClutterActor  *ws = l->data;

      nbtk_table_add_actor (strip, ws, 0, ws_count);

      ++ws_count;
      l = l->next;
    }

  clutter_actor_get_size (CLUTTER_ACTOR (strip), &w, &h);
  clutter_actor_set_position (CLUTTER_ACTOR (strip), 0, (screen_height - h)/2);

}

