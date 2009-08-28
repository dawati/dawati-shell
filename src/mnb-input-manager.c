/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-input-manager.c */
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

#include "mnb-input-manager.h"

static MnbInputManager *mgr_singleton = NULL;
static GQuark           quark_mir;

/*
 * The machinery for manipulating shape of the stage window.
 *
 * The problem: we are mixing UI done in Clutter with applications using X.
 * When the Clutter stage window receives input events, the X windows (located
 * beneath it) do not. At time we want events on parts of the screen to be
 * handled by Clutter and parts by X; for that we have to change the input
 * shape of the stage window (for which mutter provides API).
 *
 * We can use the XFixes API to modify the input shape to match our
 * requirements; the one complication in this is that while modifying the
 * existing shape is easy (a simple union or subtraction), such modification
 * cannot be trivially undone. For example, let's start with initial state (b):
 *
 *   1. All events go to X, the input shape is empty.
 *
 *   2. Notification appears requring area A to be added to the input shape.
 *
 *   3. User opens Panel; the new required shape is the union of basic state
 *      (a) and area A require by notification (a u A).
 *
 *   4. User deals with notification, which closes; at this point we need to
 *      reverse the union (a u A); unfortunatley, union operation is not
 *      reversible unless we know what the consituent parts of the union were
 *      (e.g., subtracting A from ours shape would leave us with a hole where
 *      A was).
 *
 * So, we maintain a stack of regions that make up the current input shape.
 *
 * Further problem arises from the fact that we need to be able to both add
 * areas to the input region, and punch holes in. While this is relatively
 * simple to do, we need to ensure that some regions are give higher priority
 * that others. For example, when an urgent notification pops up, we want to
 * be certain that it will receive the required input, regardless of some other
 * UI element that might request a hole in an overlaping area. We ensure this
 * by spliting the stack into layers, with request from each higher layer
 * taking precendence over any requests from lower layers.
 *
 * Within each stack layer regions are combined in the order in which they were
 * pushed on the stack. The resulting region is used as the input shape for
 * stage window.
 *
 * Each region remains on the stack until it is explicitely removed, using the
 * region ID obtained during the stack push.
 *
 * The individual functions are commented on below.
 */

static void mnb_input_manager_apply_stack (void);

struct MnbInputRegion
{
  XserverRegion region;
  gboolean      inverse;
  MnbInputLayer layer;
};

struct MnbInputManager
{
  MutterPlugin *plugin;
  GList        *layers[MNB_INPUT_LAYER_TOP + 1];
  XserverRegion current_region;
};

void
mnb_input_manager_create (MutterPlugin *plugin)
{
  /*
   * Single instance permitted.
   */
  g_assert (!mgr_singleton);

  mgr_singleton = g_new0(MnbInputManager, 1);

  mgr_singleton->plugin = plugin;

  quark_mir = g_quark_from_static_string ("MNB-INPUT-MANAGER-mir");
}

void
mnb_input_manager_destroy ()
{
  GList   *l, *o;
  gint     i;
  Display *xdpy;

  g_assert (mgr_singleton);

  xdpy = mutter_plugin_get_xdisplay (mgr_singleton->plugin);

  for (i = 0; i <= MNB_INPUT_LAYER_TOP; ++i)
    {
      l = o = mgr_singleton->layers[i];

      while (l)
        {
          MnbInputRegion *mir = l->data;

          XFixesDestroyRegion (xdpy, mir->region);

          g_slice_free (MnbInputRegion, mir);

          l = l->next;
        }

      g_list_free (o);
    }

  if (mgr_singleton->current_region)
    XFixesDestroyRegion (xdpy, mgr_singleton->current_region);

  g_free (mgr_singleton);
  mgr_singleton = NULL;
}

/*
 * mnb_input_manager_push_region ()
 *
 * Pushes region of the given dimensions onto the input region stack; this is
 * immediately reflected in the actual input shape.
 *
 * x, y, width, height: region position and size (screen-relative)
 *
 * inverse: indicates whether the region should be added to the input shape
 *          (FALSE; input events in this area will go to Clutter) or subtracted
 *          from it (TRUE; input events will go to X)
 *
 * layer: Which layer this input region belongs to
 *
 * returns: id that identifies this region; used in subsequent call to
 *          mnb_input_manager_remove_region().
 */
MnbInputRegion *
mnb_input_manager_push_region (gint          x,
                               gint          y,
                               guint         width,
                               guint         height,
                               gboolean      inverse,
                               MnbInputLayer layer)
{
  MnbInputRegion *mir  = g_slice_alloc (sizeof (MnbInputRegion));
  Display        *xdpy;
  XRectangle      rect;

  g_assert (mgr_singleton && layer >= 0 && layer <= MNB_INPUT_LAYER_TOP);

  xdpy = mutter_plugin_get_xdisplay (mgr_singleton->plugin);

  rect.x       = x;
  rect.y       = y;
  rect.width   = width;
  rect.height  = height;

  mir->inverse = inverse;
  mir->region  = XFixesCreateRegion (xdpy, &rect, 1);
  mir->layer   = layer;

  mgr_singleton->layers[layer] =
    g_list_append (mgr_singleton->layers[layer], mir);

  mnb_input_manager_apply_stack ();

  return mir;
}

/*
 * mnb_input_manager_remove_region ()
 *
 * Removes region previously pushed onto the stack with This change is
 * immediately applied to the actual input shape.
 *
 * mir: the region ID returned by mnb_input_manager_push_region().
 */
void
mnb_input_manager_remove_region (MnbInputRegion  *mir)
{
  mnb_input_manager_remove_region_without_update (mir);
  mnb_input_manager_apply_stack ();
}

/*
 * mnb_input_manager_remove_region_without_update()
 *
 * Removes region previously pushed onto the stack.  This changes does not
 * immediately filter into the actual input shape; this is useful if you need to
 * replace an existing region, as it saves round trip to the server.
 *
 * mir: the region ID returned by mnb_input_manager_push_region().
 */
void
mnb_input_manager_remove_region_without_update (MnbInputRegion *mir)
{
  Display *xdpy;

  g_assert (mgr_singleton);

  xdpy = mutter_plugin_get_xdisplay (mgr_singleton->plugin);

  if (mir->region)
    XFixesDestroyRegion (xdpy, mir->region);

  mgr_singleton->layers[mir->layer]
    = g_list_remove (mgr_singleton->layers[mir->layer], mir);

  g_slice_free (MnbInputRegion, mir);
}

/*
 * Applies the current input shape base and stack to the stage input shape.
 * This function is for internal use only and should not be used outside of the
 * actual implementation of the input shape stack.
 */
static void
mnb_input_manager_apply_stack (void)
{
  Display       *xdpy;
  GList         *l;
  gint           i;
  XserverRegion  result;

  g_assert (mgr_singleton);

  xdpy = mutter_plugin_get_xdisplay (mgr_singleton->plugin);

  if (mgr_singleton->current_region)
    XFixesDestroyRegion (xdpy, mgr_singleton->current_region);

  result = mgr_singleton->current_region = XFixesCreateRegion (xdpy, NULL, 0);

  for (i = 0; i <= MNB_INPUT_LAYER_TOP; ++i)
    {
      l = mgr_singleton->layers[i];

      if (!l)
        continue;

      while (l)
        {
          MnbInputRegion *mir = l->data;

          if (mir->inverse)
            XFixesSubtractRegion (xdpy, result, result, mir->region);
          else
            XFixesUnionRegion (xdpy, result, result, mir->region);

          l = l->next;
        }
    }

  mutter_plugin_set_stage_input_region (mgr_singleton->plugin, result);
}

static void
mcw_allocation_cb (ClutterActor *actor, GParamSpec *pspec, gpointer data)
{
  ClutterGeometry  geom;
  MnbInputRegion  *mir = g_object_get_qdata (G_OBJECT (actor), quark_mir);
  XserverRegion    rgn;
  XRectangle       rect;
  Display         *xdpy;

  g_assert (mgr_singleton);

  if (!mir)
    return;

  xdpy = mutter_plugin_get_xdisplay (mgr_singleton->plugin);

  rgn = mir->region;

  clutter_actor_get_geometry (actor, &geom);

  rect.x      = geom.x;
  rect.y      = geom.y;
  rect.width  = geom.width;
  rect.height = geom.height;

  XFixesSetRegion (xdpy, mir->region, &rect, 1);

  mnb_input_manager_apply_stack ();
}

static void
mcw_hide_cb (ClutterActor *actor, gpointer data)
{
  MnbInputRegion *mir = g_object_get_qdata (G_OBJECT (actor), quark_mir);

  if (!mir)
    return;

  g_object_set_qdata (G_OBJECT (actor), quark_mir, NULL);

  mnb_input_manager_remove_region (mir);
}

static void
mcw_show_cb (ClutterActor *actor, MnbInputLayer layer)
{
  ClutterGeometry  geom;
  MnbInputRegion  *mir  = g_object_get_qdata (G_OBJECT (actor), quark_mir);
  Display         *xdpy;

  g_assert (mgr_singleton);

  xdpy = mutter_plugin_get_xdisplay (mgr_singleton->plugin);

  clutter_actor_get_geometry (actor, &geom);

  if (!mir)
    {
      mir = mnb_input_manager_push_region (geom.x, geom.y,
                                           geom.width, geom.height,
                                           TRUE, layer);

      g_object_set_qdata (G_OBJECT (actor), quark_mir, mir);
    }
  else
    {
      XserverRegion rgn;
      XRectangle    rect;

      rgn = mir->region;

      rect.x      = geom.x;
      rect.y      = geom.y;
      rect.width  = geom.width;
      rect.height = geom.height;

      XFixesSetRegion (xdpy, mir->region, &rect, 1);

      mnb_input_manager_apply_stack ();
    }
}

static void
mcw_destroy_cb (ClutterActor *actor, gpointer data)
{
  MnbInputRegion *mir = g_object_get_qdata (G_OBJECT (actor), quark_mir);

  if (!mir)
    return;

  g_object_set_qdata (G_OBJECT (actor), quark_mir, NULL);

  mnb_input_manager_remove_region (mir);
}

/*
 * Pushes region associated with MutterWindow onto the given layer of the input
 * stack, connecting appropriate singnal handlers to deal with window showing,
 * hiding, resizing and destruction.
 */
void
mnb_input_manager_push_window (MutterWindow *mcw, MnbInputLayer layer)
{
  ClutterActor    *actor = (ClutterActor*)mcw;
  ClutterGeometry  geom;
  MnbInputRegion  *mir;

  g_assert (mgr_singleton);

  clutter_actor_get_geometry (actor, &geom);

  mir = mnb_input_manager_push_region (geom.x, geom.y, geom.width, geom.height,
                                       TRUE, layer);

  g_object_set_qdata (G_OBJECT (actor), quark_mir, mir);

  g_signal_connect (actor, "notify::allocation",
                    G_CALLBACK (mcw_allocation_cb), NULL);
  g_signal_connect (actor, "show",
                    G_CALLBACK (mcw_show_cb), GINT_TO_POINTER (layer));
  g_signal_connect (actor, "hide",
                    G_CALLBACK (mcw_hide_cb), NULL);
  g_signal_connect (actor, "destroy",
                    G_CALLBACK (mcw_destroy_cb), NULL);
}
