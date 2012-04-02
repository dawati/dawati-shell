/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Dawati Netbook
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
#include "mnb-alttab-overlay.h"
#include "mnb-alttab-overlay-private.h"
#include "mnb-alttab-overlay-app.h"
#include "mnb-alttab-keys.h"
#include "../dawati-netbook.h"
#include <meta/display.h>
#include <meta/keybindings.h>
#include <X11/keysym.h>
#include <clutter/x11/clutter-x11.h>

#include <math.h>

#define PADDING 10

enum
{
  PROP_0 = 0,
};

G_DEFINE_TYPE (MnbAlttabOverlay, mnb_alttab_overlay, MX_TYPE_WIDGET);

#define MNB_ALTTAB_OVERLAY_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_ALTTAB_OVERLAY,\
                                MnbAlttabOverlayPrivate))

static void
mnb_alttab_overlay_set_property (GObject      *gobject,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_alttab_overlay_get_property (GObject    *gobject,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static gint
sort_windows_by_user_time (gconstpointer a, gconstpointer b)
{
  MetaWindowActor *m1 = (MetaWindowActor*) a;
  MetaWindowActor *m2 = (MetaWindowActor*) b;
  MetaWindow   *w1 = meta_window_actor_get_meta_window (m1);
  MetaWindow   *w2 = meta_window_actor_get_meta_window (m2);
  guint32       t1 = meta_window_get_user_time (w1);
  guint32       t2 = meta_window_get_user_time (w2);

  if (t1 > t2)
    return -1;
  else if (t2 > t1)
    return 1;

  return 0;
}

GList *
mnb_alttab_overlay_get_app_list (MnbAlttabOverlay *self)
{
  MetaPlugin   *plugin;
  MetaScreen   *screen;
  GList        *l, *filtered = NULL;

  plugin = dawati_netbook_get_plugin_singleton ();
  screen = meta_plugin_get_screen (plugin);

  for (l = meta_get_window_actors (screen); l; l = l->next)
    {
      MetaWindowActor *m = l->data;
      MetaWindow      *w = meta_window_actor_get_meta_window (m);
      MetaWindowType type;

      type = meta_window_get_window_type (w);

      if (meta_window_is_on_all_workspaces (w)   ||
          meta_window_actor_is_override_redirect (m) || /* Unecessary */
          (type != META_WINDOW_NORMAL  &&
           type != META_WINDOW_DIALOG))
        {
          continue;
        }

      /*
       * Only intersted in top-level dialogs, i.e., those not transient
       * to anything or to root.
       */
      if (type ==  META_WINDOW_DIALOG)
        {
          MetaWindow *p = meta_window_find_root_ancestor (w);

          if (p != w)
            continue;
        }

      filtered = g_list_prepend (filtered, m);
    }

  if (!filtered || filtered->next == NULL)
    {
      g_list_free (filtered);
      return NULL;
    }

  /*
   * Sort the windows in MRU order
   */
  filtered = g_list_sort (filtered, sort_windows_by_user_time);

  return filtered;
}

/*
 * If there are less that 2 applications, no population is done, and return
 * value is FALSE.
 */
static gboolean
mnb_alttab_overlay_populate (MnbAlttabOverlay *self)
{
  MnbAlttabOverlayPrivate   *priv = self->priv;
  GList                     *l, *filtered = NULL, *active = NULL;

  filtered = mnb_alttab_overlay_get_app_list (self);

  if (!filtered || filtered->next == NULL)
    {
      g_list_free (filtered);
      return FALSE;
    }

  for (l = filtered; l; l = l->next)
    {
      MetaWindowActor     *m = l->data;
      MnbAlttabOverlayApp *app;

      app = mnb_alttab_overlay_app_new (m);

      if (!active)
        active = l->next;

      /*
       * Mark second application active.
       */
      if (l == active)
        {
          mnb_alttab_overlay_app_set_active (app, TRUE);
          priv->active = app;
        }

      clutter_actor_add_child (priv->grid, (ClutterActor *) app);
    }

  g_list_free (filtered);

  return TRUE;
}

static void
mnb_alttab_overlay_depopulate (MnbAlttabOverlay *self)
{
  MnbAlttabOverlayPrivate *priv = self->priv;
  ClutterActorIter iter;
  ClutterActor *child;

  clutter_actor_iter_init (&iter, priv->grid);
  while (clutter_actor_iter_next (&iter, &child))
    clutter_actor_iter_destroy (&iter);
}

static void
mnb_alttab_overlay_kbd_grab_notify_cb (MetaScreen       *screen,
                                       GParamSpec       *pspec,
                                       MnbAlttabOverlay *overlay)
{
  MnbAlttabOverlayPrivate *priv = overlay->priv;
  gboolean                 grabbed;

  /* MNB_DBG_MARK(); */

  if (!priv->in_alt_grab)
    return;

  g_object_get (screen, "keyboard-grabbed", &grabbed, NULL);

  /*
   * If the property has changed to FALSE, i.e., Mutter just called
   * XUngrabKeyboard(), reset the flag
   */
  if (!grabbed)
    {
      priv->in_alt_grab = FALSE;

      mnb_alttab_overlay_hide (overlay);
    }
}

static void
mnb_alttab_overlay_constructed (GObject *self)
{
  MnbAlttabOverlayPrivate *priv   = MNB_ALTTAB_OVERLAY (self)->priv;
  MetaPlugin              *plugin = dawati_netbook_get_plugin_singleton ();
  MxGrid                  *grid;
  MxScrollView            *scrollview;

  if (G_OBJECT_CLASS (mnb_alttab_overlay_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_alttab_overlay_parent_class)->constructed (self);

  priv->scrollview = mx_scroll_view_new ();
  scrollview = (MxScrollView *) priv->scrollview;
  mx_scroll_view_set_enable_mouse_scrolling (scrollview, FALSE);
  mx_scroll_view_set_scroll_policy (scrollview, MX_SCROLL_POLICY_VERTICAL);

  priv->grid = mx_grid_new ();
  grid = (MxGrid *) priv->grid;

  mx_bin_set_child (MX_BIN (scrollview), priv->grid);
  clutter_actor_add_child ((ClutterActor *) self, priv->scrollview);

  mx_grid_set_column_spacing (grid, MNB_ALTTAB_OVERLAY_TILE_SPACING);
  mx_grid_set_row_spacing (grid, MNB_ALTTAB_OVERLAY_TILE_SPACING);
  mx_grid_set_child_x_align (grid, MX_ALIGN_MIDDLE);
  mx_grid_set_child_y_align (grid, MX_ALIGN_MIDDLE);

  mx_stylable_set_style_class (MX_STYLABLE (self),"alttab-overlay");

  g_signal_connect (meta_plugin_get_screen (plugin),
                    "notify::keyboard-grabbed",
                    G_CALLBACK (mnb_alttab_overlay_kbd_grab_notify_cb),
                    self);
}

static void
mnb_alttab_overlay_get_preferred_width (ClutterActor *self,
                                        gfloat        for_height,
                                        gfloat       *min_width_p,
                                        gfloat       *natural_width_p)
{
  MnbAlttabOverlayPrivate *priv    = MNB_ALTTAB_OVERLAY (self)->priv;
  MxPadding                padding = { 0, };
  gfloat                   width;

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  clutter_actor_get_preferred_width (priv->scrollview,
                                     for_height - padding.left - padding.right,
                                     NULL, &width);

  width += padding.left + padding.bottom;
  width = MIN (width, priv->screen_width);

  if (min_width_p)
    *min_width_p = width;

  if (natural_width_p)
    *natural_width_p = width;
}

static void
mnb_alttab_overlay_get_preferred_height (ClutterActor *self,
                                         gfloat        for_width,
                                         gfloat       *min_height_p,
                                         gfloat       *natural_height_p)
{
  MnbAlttabOverlayPrivate *priv     = MNB_ALTTAB_OVERLAY (self)->priv;
  MxPadding                padding  = { 0, };
  gfloat                   height;

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  /*
   * We allow at most MNB_ALLTAB_OVERLAY_ROWS visible
   */
  clutter_actor_get_preferred_height (priv->grid,
                                      for_width - padding.left - padding.right,
                                      NULL, &height);

  height += (padding.top + padding.bottom);
  height = MIN (height, priv->screen_height);

  if (min_height_p)
    *min_height_p = height;

  if (natural_height_p)
    *natural_height_p = height;
}

static void
mnb_alttab_overlay_paint (ClutterActor *self)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (self)->priv;
  ClutterGeometry geom;

  CLUTTER_ACTOR_CLASS (mnb_alttab_overlay_parent_class)->paint (self);

  clutter_actor_get_allocation_geometry (priv->grid, &geom);

  clutter_actor_paint (priv->scrollview);
}

static void
mnb_alttab_overlay_allocate (ClutterActor          *actor,
                             const ClutterActorBox *box,
                             ClutterAllocationFlags flags)
{
  MnbAlttabOverlayPrivate *priv    = MNB_ALTTAB_OVERLAY (actor)->priv;
  MxPadding                padding = { 0, };
  ClutterActorBox          child_box;

  /*
   * Let the parent class do it's thing, and then allocate for the icon.
   */
  CLUTTER_ACTOR_CLASS (mnb_alttab_overlay_parent_class)->allocate (actor,
                                                                   box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  child_box.x1 = padding.left;
  child_box.y1 = padding.top;
  child_box.x2 = MAX (child_box.x1, child_box.x1 + (box->x2 - box->x1) - padding.right);
  child_box.y2 = MAX (child_box.y1, child_box.y1 + (box->y2 - box->y1) - padding.bottom);

  clutter_actor_allocate (priv->scrollview, &child_box, flags);
}

static void
mnb_alttab_overlay_class_init (MnbAlttabOverlayClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);

  object_class->get_property         = mnb_alttab_overlay_get_property;
  object_class->set_property         = mnb_alttab_overlay_set_property;
  object_class->constructed          = mnb_alttab_overlay_constructed;

  actor_class->get_preferred_width   = mnb_alttab_overlay_get_preferred_width;
  actor_class->get_preferred_height  = mnb_alttab_overlay_get_preferred_height;
  actor_class->paint                 = mnb_alttab_overlay_paint;
  actor_class->allocate              = mnb_alttab_overlay_allocate;

  g_type_class_add_private (klass, sizeof (MnbAlttabOverlayPrivate));
}

static void
mnb_alttab_overlay_init (MnbAlttabOverlay *self)
{
  self->priv = MNB_ALTTAB_OVERLAY_GET_PRIVATE (self);

  mnb_alttab_overlay_setup_metacity_keybindings (self);
}

MnbAlttabOverlay *
mnb_alttab_overlay_new (void)
{
  return g_object_new (MNB_TYPE_ALTTAB_OVERLAY, NULL);
}

/*
 * Activates the window currently selected in the overlay.
 */
void
mnb_alttab_overlay_activate_selection (MnbAlttabOverlay *overlay,
                                       guint             timestamp)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (overlay)->priv;
  MnbAlttabOverlayApp     *active = priv->active;
  MetaWindowActor         *mcw;

  g_return_if_fail (active);

  priv->active = NULL;

  mcw = mnb_alttab_overlay_app_get_mcw (active);

  g_return_if_fail (mcw);

  if (CLUTTER_ACTOR_IS_VISIBLE (overlay))
    mnb_alttab_overlay_hide (overlay);


#if 0
  /* FIXME -- if we use some kind of an animation to show/hide, we probably want
   * the animation to finish before we activate the workspace switch.
   */
  g_signal_connect (overlay, "hide-completed",
                    G_CALLBACK (mnb_alttab_hide_completed_cb), overlay);
#endif

  mnb_alttab_overlay_activate_window (overlay, mcw, timestamp);
}

/*
 * Handles X button and pointer events during Alt_tab grab.
 *
 * Returns TRUE if handled.
 */
gboolean
mnb_alttab_overlay_handle_xevent (MnbAlttabOverlay *overlay, XEvent *xev)
{
  MnbAlttabOverlayPrivate *priv   = overlay->priv;
  MetaPlugin              *plugin = dawati_netbook_get_plugin_singleton ();

  if (!priv->in_alt_grab)
    return FALSE;

  if (xev->type == KeyRelease)
    {
      if ((XKeycodeToKeysym (xev->xkey.display,
                             xev->xkey.keycode, 0) == XK_Alt_L) ||
          (XKeycodeToKeysym (xev->xkey.display,
                             xev->xkey.keycode, 0) == XK_Alt_R))
        {
          MetaScreen   *screen  = meta_plugin_get_screen (plugin);
          MetaDisplay  *display = meta_screen_get_display (screen);
          Time          timestamp = xev->xkey.time;

          meta_display_end_grab_op (display, timestamp);
          priv->in_alt_grab = FALSE;

          mnb_alttab_overlay_activate_selection (overlay, timestamp);
        }

      /*
       * Block further processing.
       */
      return TRUE;
    }
  else
    {
      /*
       * Block processing of all keyboard and pointer events.
       */
      if (xev->type == KeyPress      ||
          xev->type == ButtonPress   ||
          xev->type == ButtonRelease ||
          xev->type == MotionNotify)
        return TRUE;
    }

  return FALSE;
}

void
mnb_alttab_overlay_advance (MnbAlttabOverlay *overlay, gboolean backward)
{
  MnbAlttabOverlayPrivate *priv = overlay->priv;
  GList                   *children, *l;
  gboolean                 next_is_active = FALSE;

  children = clutter_container_get_children (CLUTTER_CONTAINER (priv->grid));

  for (l = backward ? g_list_last (children) : children;
       l;
       l = backward ? l->prev : l->next)
    {
      MnbAlttabOverlayApp *app    = l->data;
      gboolean             active = mnb_alttab_overlay_app_get_active (app);

      if (active)
        {
          next_is_active = TRUE;
          mnb_alttab_overlay_app_set_active (app, FALSE);
          priv->active = app;
        }
      else if (next_is_active)
        {
          next_is_active = FALSE;
          mnb_alttab_overlay_app_set_active (app, TRUE);
          priv->active = app;
        }
    }

  if (next_is_active)
    {
      /*
       * The previously active app was at the end of the list.
       */
      MnbAlttabOverlayApp *app =
        backward ? (g_list_last (children))->data : children->data;
      ClutterActorBox box;
      ClutterGeometry geom;

      clutter_actor_get_allocation_box (CLUTTER_ACTOR (app), &box);

      geom.x = box.x1;
      geom.y = box.y1;
      geom.width = box.x2 - box.x1;
      geom.height = box.y2 - box.y1;

      mnb_alttab_overlay_app_set_active (app, TRUE);
      mx_scroll_view_ensure_visible (MX_SCROLL_VIEW (priv->scrollview), &geom);
      priv->active = app;
    }

  g_list_free (children);
}

gboolean
mnb_alttab_overlay_tab_still_down (MnbAlttabOverlay *overlay)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (overlay)->priv;
  MetaPlugin              *plugin  = dawati_netbook_get_plugin_singleton ();
  MetaScreen              *screen  = meta_plugin_get_screen (plugin);
  MetaDisplay             *display = meta_screen_get_display (screen);
  Display                 *xdpy    = meta_display_get_xdisplay (display);
  char                     keys[32];
  KeyCode                  code_tab, code_shift_l, code_shift_r;
  guint                    bit_index_t, byte_index_t;
  guint                    bit_index_sl, byte_index_sl;
  guint                    bit_index_sr, byte_index_sr;

  code_tab     = XKeysymToKeycode (xdpy, XK_Tab);
  code_shift_l = XKeysymToKeycode (xdpy, XK_Shift_L);
  code_shift_r = XKeysymToKeycode (xdpy, XK_Shift_R);

  g_return_val_if_fail (code_tab != NoSymbol, FALSE);

  byte_index_t = code_tab / 8;
  bit_index_t  = code_tab & 7;

  byte_index_sl = code_shift_l / 8;
  bit_index_sl  = code_shift_l & 7;

  byte_index_sr = code_shift_r / 8;
  bit_index_sr  = code_shift_r & 7;

  XQueryKeymap (xdpy, &keys[0]);

  if ((1 & (keys[byte_index_sl] >> bit_index_sl)) ||
      (1 & (keys[byte_index_sr] >> bit_index_sr)))
    {
      priv->backward = TRUE;
    }
  else
    {
      priv->backward = FALSE;
    }

  return (1 & (keys[byte_index_t] >> bit_index_t));
}

/*
 * Returns FALSE if there is nothing worth showing.
 */
gboolean
mnb_alttab_overlay_show (MnbAlttabOverlay *overlay, gboolean backward)
{
  MnbAlttabOverlayPrivate *priv   = overlay->priv;
  MetaPlugin              *plugin = dawati_netbook_get_plugin_singleton ();
  MetaScreen              *screen = meta_plugin_get_screen (plugin);
  MxPadding                padding = { 0, };
  gfloat                   w, h;
  gint                     max_nb_tiles;
  gfloat                   spacing, tile_width;

  /* FIXME -- do an animation */
  if (!mnb_alttab_overlay_populate (overlay))
    return FALSE;

  meta_screen_get_size (screen, &priv->screen_width, &priv->screen_height);

  /*
   * Must ensure size, otherewise the reported actor size is not acurate.
   */
  mx_stylable_style_changed (MX_STYLABLE (overlay), MX_STYLE_CHANGED_FORCE);

  /**/
  mx_widget_get_padding (MX_WIDGET (overlay), &padding);

  spacing = mx_grid_get_column_spacing (MX_GRID (priv->grid));

  /* So bad... */
  tile_width = mpl_application_get_tile_width ();

  max_nb_tiles = floorf (((priv->screen_width - padding.left - padding.right)
                          + spacing) / (tile_width + spacing));

  if (max_nb_tiles < 0)
    max_nb_tiles = 1;

  mx_grid_set_max_stride (MX_GRID (priv->grid), max_nb_tiles);
  /**/

  clutter_actor_get_preferred_size ((ClutterActor*) overlay,
                                    NULL, NULL,
                                    &w, &h);

  clutter_actor_set_position ((ClutterActor*) overlay,
                              (priv->screen_width - w)/2,
                              (priv->screen_height - h)/2);

  clutter_actor_show ((ClutterActor*)overlay);

  return TRUE;
}

void
mnb_alttab_overlay_hide (MnbAlttabOverlay *overlay)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (overlay)->priv;

  if (priv->slowdown_timeout_id)
    {
      g_source_remove (priv->slowdown_timeout_id);
      priv->slowdown_timeout_id = 0;
    }

  /* FIXME -- do an animation */
  clutter_actor_hide ((ClutterActor*)overlay);
  mnb_alttab_overlay_depopulate (overlay);
}

/*
 * Activates the given MutterWindow, taking care of the quirks in Meta API.
 */
void
mnb_alttab_overlay_activate_window (MnbAlttabOverlay *overlay,
                                    MetaWindowActor  *activate,
                                    guint             timestamp)
{
  MnbAlttabOverlayPrivate *priv = overlay->priv;
  MetaWindow              *next;
  MetaWorkspace           *workspace;
  MetaWorkspace           *active_workspace;
  MetaScreen              *screen;

  priv->in_alt_grab = FALSE;

  next = meta_window_actor_get_meta_window (activate);

  g_return_if_fail (next);

  screen           = meta_window_get_screen (next);
  workspace        = meta_window_get_workspace (next);
  active_workspace = meta_screen_get_active_workspace (screen);

  g_return_if_fail (workspace);

  if (!active_workspace || (active_workspace == workspace))
    {
      meta_window_activate_with_workspace (next, timestamp, workspace);
    }
  else
    {
      meta_workspace_activate_with_focus (workspace, next, timestamp);
    }
}
