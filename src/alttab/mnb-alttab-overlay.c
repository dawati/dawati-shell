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
#include "mnb-alttab-overlay.h"
#include "mnb-alttab-overlay-private.h"
#include "mnb-alttab-overlay-app.h"
#include "mnb-alttab-keys.h"
#include "../meego-netbook.h"
#include <display.h>
#include <keybindings.h>
#include <X11/keysym.h>
#include <clutter/x11/clutter-x11.h>

#define PADDING 10

static void mnb_alttab_stop_autoscroll (MnbAlttabOverlay *overlay);

enum
{
  PROP_0 = 0,

  PROP_SCROLL_Y
};

G_DEFINE_TYPE (MnbAlttabOverlay, mnb_alttab_overlay, MX_TYPE_WIDGET);

#define MNB_ALTTAB_OVERLAY_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_ALTTAB_OVERLAY,\
                                MnbAlttabOverlayPrivate))

static void
mnb_alttab_overlay_dispose (GObject *object)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  clutter_actor_destroy (priv->grid);
  priv->grid = NULL;

  G_OBJECT_CLASS (mnb_alttab_overlay_parent_class)->dispose (object);
}

static void
mnb_alltab_overlay_set_scroll_y (MnbAlttabOverlay *overlay, gfloat scroll_y)
{
  MnbAlttabOverlayPrivate *priv = overlay->priv;

  if (priv->scroll_y == scroll_y)
    return;

  priv->scroll_y = scroll_y;
  clutter_actor_queue_redraw ((ClutterActor*)overlay);
}

static void
mnb_alttab_overlay_set_property (GObject      *gobject,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  /* MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (gobject)->priv; */

  switch (prop_id)
    {
    case PROP_SCROLL_Y:
      mnb_alltab_overlay_set_scroll_y (MNB_ALTTAB_OVERLAY (gobject),
                                       g_value_get_float (value));
      break;

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
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (gobject)->priv;

  switch (prop_id)
    {
    case PROP_SCROLL_Y:
      g_value_set_float (value, priv->scroll_y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static gint
sort_windows_by_user_time (gconstpointer a, gconstpointer b)
{
  MutterWindow *m1 = (MutterWindow*) a;
  MutterWindow *m2 = (MutterWindow*) b;
  MetaWindow   *w1 = mutter_window_get_meta_window (m1);
  MetaWindow   *w2 = mutter_window_get_meta_window (m2);
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
  MutterPlugin *plugin;
  MetaScreen   *screen;
  GList        *l, *filtered = NULL;

  plugin = meego_netbook_get_plugin_singleton ();
  screen = mutter_plugin_get_screen (plugin);

  for (l = mutter_get_windows (screen); l; l = l->next)
    {
      MutterWindow *m = l->data;
      MetaWindow   *w = mutter_window_get_meta_window (m);
      MetaCompWindowType type;

      type = mutter_window_get_window_type (m);

      if (meta_window_is_on_all_workspaces (w)   ||
          mutter_window_is_override_redirect (m) || /* Unecessary */
          (type != META_COMP_WINDOW_NORMAL  &&
           type != META_COMP_WINDOW_DIALOG))
        {
          continue;
        }

      /*
       * Only intersted in top-level dialogs, i.e., those not transient
       * to anything or to root.
       */
      if (type ==  META_COMP_WINDOW_DIALOG)
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
  MutterPlugin               *plugin = meego_netbook_get_plugin_singleton ();
  MeegoNetbookPluginPrivate *ppriv  = MEEGO_NETBOOK_PLUGIN (plugin)->priv;
  MnbAlttabOverlayPrivate    *priv = self->priv;
  GList                      *l, *filtered = NULL, *active = NULL;

  filtered = mnb_alttab_overlay_get_app_list (self);

  if (!filtered || filtered->next == NULL)
    {
      g_list_free (filtered);
      return FALSE;
    }

  for (l = filtered; l; l = l->next)
    {
      MutterWindow          *m = l->data;
      MnbAlttabOverlayApp *app;

      app = mnb_alttab_overlay_app_new (m, ppriv->desktop_tex);

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

      clutter_container_add_actor (CLUTTER_CONTAINER (priv->grid),
                                   (ClutterActor*) app);
    }

  g_list_free (filtered);

  return TRUE;
}

static void
mnb_alttab_overlay_depopulate (MnbAlttabOverlay *self)
{
  MnbAlttabOverlayPrivate *priv = self->priv;

  clutter_container_foreach (CLUTTER_CONTAINER (priv->grid),
                             (ClutterCallback)clutter_actor_destroy,
                             NULL);
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
  MxGrid                  *grid   = MX_GRID (mx_grid_new ());
  MutterPlugin            *plugin = meego_netbook_get_plugin_singleton ();

  if (G_OBJECT_CLASS (mnb_alttab_overlay_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_alttab_overlay_parent_class)->constructed (self);

  priv->grid = (ClutterActor*)grid;

  clutter_actor_set_parent ((ClutterActor*)grid, (ClutterActor*)self);

  mx_grid_set_max_stride (grid, MNB_ALLTAB_OVERLAY_COLUMNS);

  mx_grid_set_column_spacing (grid, MNB_ALTTAB_OVERLAY_TILE_SPACING);
  mx_grid_set_row_spacing (grid, MNB_ALTTAB_OVERLAY_TILE_SPACING);
  mx_grid_set_child_x_align (grid, MX_ALIGN_MIDDLE);
  mx_grid_set_child_y_align (grid, MX_ALIGN_MIDDLE);

  mx_stylable_set_style_class (MX_STYLABLE (self),"alttab-overlay");

  g_signal_connect (mutter_plugin_get_screen (plugin),
                    "notify::keyboard-grabbed",
                    G_CALLBACK (mnb_alttab_overlay_kbd_grab_notify_cb),
                    self);
}

static void
mnb_alttab_overlay_map (ClutterActor *self)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_alttab_overlay_parent_class)->map (self);

  if (priv->grid)
    clutter_actor_map (priv->grid);
}

static void
mnb_alttab_overlay_unmap (ClutterActor *self)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_alttab_overlay_parent_class)->unmap (self);

  if (priv->grid)
    clutter_actor_unmap (priv->grid);
}

static void
mnb_alttab_overlay_get_preferred_width (ClutterActor *self,
                                        gfloat        for_height,
                                        gfloat       *min_width_p,
                                        gfloat       *natural_width_p)
{
  MnbAlttabOverlayPrivate *priv    = MNB_ALTTAB_OVERLAY (self)->priv;
  MxPadding                padding = { 0, };

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  clutter_actor_get_preferred_width (priv->grid,
                                     for_height,
                                     min_width_p,
                                     natural_width_p);

  if (min_width_p)
    *min_width_p += (padding.left + padding.right);

  if (natural_width_p)
    *natural_width_p += (padding.left + padding.right);
}

static void
mnb_alttab_overlay_get_preferred_height (ClutterActor *self,
                                         gfloat        for_width,
                                         gfloat       *min_height_p,
                                         gfloat       *natural_height_p)
{
  MnbAlttabOverlayPrivate *priv     = MNB_ALTTAB_OVERLAY (self)->priv;
  MxPadding                padding  = { 0, };
  MxPadding                gpadding = { 0, };
  gfloat                   grid_n_h;
  gfloat                   height;

  mx_widget_get_padding (MX_WIDGET (self), &padding);
  mx_widget_get_padding (MX_WIDGET (priv->grid), &gpadding);

  /*
   * We allow at most MNB_ALLTAB_OVERLAY_ROWS visible
   */
  height = MNB_ALTTAB_OVERLAY_TILE_HEIGHT * MNB_ALLTAB_OVERLAY_ROWS +
    MNB_ALTTAB_OVERLAY_TILE_SPACING * (MNB_ALLTAB_OVERLAY_ROWS - 1) +
    gpadding.top + gpadding.bottom + padding.top + padding.bottom;

  clutter_actor_get_preferred_height (priv->grid,
                                      for_width - padding.left - padding.right,
                                      NULL, &grid_n_h);

  grid_n_h += (padding.top + padding.bottom);

  if (grid_n_h < height)
    height = grid_n_h;

  if (min_height_p)
    *min_height_p = height;

  if (natural_height_p)
    *natural_height_p = height;

  priv->viewport_height = height - padding.top - padding.bottom;
}

static void
mnb_alttab_overlay_paint (ClutterActor *self)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_alttab_overlay_parent_class)->paint (self);

  if (priv->grid && CLUTTER_ACTOR_IS_MAPPED (priv->grid))
    {
      ClutterGeometry geom;

      clutter_actor_get_allocation_geometry (priv->grid, &geom);

#if CLUTTER_CHECK_VERSION (1, 1, 3)
      cogl_clip_push_rectangle (geom.x, geom.y,
                                geom.x + geom.width,
                                geom.y + priv->viewport_height);
#else
      cogl_clip_push (geom.x, geom.y, geom.width, priv->viewport_height);
#endif

      cogl_push_matrix ();
      cogl_translate (0, (gint) priv->scroll_y * -1, 0);

      clutter_actor_paint (priv->grid);

      cogl_pop_matrix ();
      cogl_clip_pop ();
    }
}

static void
mnb_alttab_overlay_allocate (ClutterActor          *actor,
                             const ClutterActorBox *box,
                             ClutterAllocationFlags flags)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (actor)->priv;

  /*
   * Let the parent class do it's thing, and then allocate for the icon.
   */
  CLUTTER_ACTOR_CLASS (mnb_alttab_overlay_parent_class)->allocate (actor,
                                                                   box, flags);

  if (priv->grid)
    {
      MxPadding       padding    = { 0, };
      gfloat          w, h;
      ClutterActorBox box;

      mx_widget_get_padding (MX_WIDGET (actor), &padding);

      clutter_actor_get_preferred_size (priv->grid, NULL, NULL,
                                        &w, &h);

      box.x1 = padding.left;
      box.y1 = padding.top;
      box.x2 = padding.left + w;
      box.y2 = padding.top + h;

      clutter_actor_allocate (priv->grid, &box, flags);
    }
}

static void
mnb_alttab_overlay_class_init (MnbAlttabOverlayClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec        *pspec;

  object_class->dispose              = mnb_alttab_overlay_dispose;
  object_class->get_property         = mnb_alttab_overlay_get_property;
  object_class->set_property         = mnb_alttab_overlay_set_property;
  object_class->constructed          = mnb_alttab_overlay_constructed;

  actor_class->map                   = mnb_alttab_overlay_map;
  actor_class->unmap                 = mnb_alttab_overlay_unmap;
  actor_class->get_preferred_width   = mnb_alttab_overlay_get_preferred_width;
  actor_class->get_preferred_height  = mnb_alttab_overlay_get_preferred_height;
  actor_class->paint                 = mnb_alttab_overlay_paint;
  actor_class->allocate              = mnb_alttab_overlay_allocate;

  g_type_class_add_private (klass, sizeof (MnbAlttabOverlayPrivate));

  pspec = g_param_spec_float ("scroll-y",
                              "scroll y",
                              "scroll y coordinate of overlay viewport",
                              0.0, G_MAXFLOAT,
                              0.0,
                              G_PARAM_READWRITE);

  g_object_class_install_property (object_class, PROP_SCROLL_Y, pspec);
}

static void
mnb_alttab_overlay_init (MnbAlttabOverlay *self)
{
  MnbAlttabOverlayPrivate *priv;

  priv = self->priv = MNB_ALTTAB_OVERLAY_GET_PRIVATE (self);

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
  MutterWindow            *mcw;

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
  MutterPlugin            *plugin = meego_netbook_get_plugin_singleton ();

  if (!priv->in_alt_grab)
    return FALSE;

  if (xev->type == KeyRelease)
    {
      if ((XKeycodeToKeysym (xev->xkey.display,
                             xev->xkey.keycode, 0) == XK_Alt_L) ||
          (XKeycodeToKeysym (xev->xkey.display,
                             xev->xkey.keycode, 0) == XK_Alt_R))
        {
          MetaScreen   *screen  = mutter_plugin_get_screen (plugin);
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

static void
mnb_alttab_overlay_fix_active_row (MnbAlttabOverlay *overlay, GList *children)
{
  MnbAlttabOverlayPrivate *priv = overlay->priv;
  gint                     pos;
  GList                   *l;
  guint                    current_row = priv->current_row;
  gboolean                 found = FALSE;
  gfloat                   scroll_y = priv->scroll_y;

  for (l = children, pos = 0; l; l = l->next, ++pos)
    {
      MnbAlttabOverlayApp *app    = l->data;
      gboolean             active = mnb_alttab_overlay_app_get_active (app);

      if (active)
        {
          priv->current_row = pos / MNB_ALLTAB_OVERLAY_COLUMNS;
          found = TRUE;
          break;
        }
    }

  if (!found)
    priv->current_row = 0;

  if (priv->current_row > current_row && priv->current_row > 3)
    {
      scroll_y += (MNB_ALTTAB_OVERLAY_TILE_HEIGHT +
                   MNB_ALTTAB_OVERLAY_TILE_SPACING);
    }
  else if (priv->current_row < current_row && current_row > 3)
    {
      scroll_y -= (MNB_ALTTAB_OVERLAY_TILE_HEIGHT +
                   MNB_ALTTAB_OVERLAY_TILE_SPACING);
    }

  if (priv->scroll_y < 0.0)
    {
      g_warning (G_STRLOC ": scroll out of bounds, fixing");
      priv->scroll_y = 0.0;
    }

  if (scroll_y != priv->scroll_y)
    {
      clutter_actor_animate (CLUTTER_ACTOR (overlay),
                             CLUTTER_EASE_IN_SINE,
                             MNB_ALTTAB_OVERLAY_SCROLL_DURATION,
                             "scroll-y", scroll_y,
                             NULL);
    }
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
          priv->active = FALSE;
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

      mnb_alttab_overlay_app_set_active (app, TRUE);
      priv->active = app;
    }

  mnb_alttab_overlay_fix_active_row (overlay, children);

  g_list_free (children);
}

gboolean
mnb_alttab_overlay_tab_still_down (MnbAlttabOverlay *overlay)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (overlay)->priv;
  MutterPlugin            *plugin  = meego_netbook_get_plugin_singleton ();
  MetaScreen              *screen  = mutter_plugin_get_screen (plugin);
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

static gboolean
mnb_alttab_overlay_autoscroll_advance_cb (gpointer data)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (data)->priv;

  if (mnb_alttab_overlay_tab_still_down (data))
    mnb_alttab_overlay_advance (data, priv->backward);
  else
    mnb_alttab_stop_autoscroll (data);

  return TRUE;
}

static gboolean
mnb_alttab_overlay_autoscroll_trigger_cb (gpointer data)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (data)->priv;

  priv->autoscroll_trigger_id = 0;

  g_assert (!priv->autoscroll_advance_id);

  if (mnb_alttab_overlay_tab_still_down (data))
    {
      mnb_alttab_overlay_advance (data, priv->backward);

      priv->autoscroll_advance_id =
        g_timeout_add (AUTOSCROLL_ADVANCE_TIMEOUT,
                       mnb_alttab_overlay_autoscroll_advance_cb,
                       data);
    }

  return FALSE;
}

/*
 * Returns FALSE if there is nothing worth showing.
 */
gboolean
mnb_alttab_overlay_show (MnbAlttabOverlay *overlay, gboolean backward)
{
  MutterPlugin  *plugin = meego_netbook_get_plugin_singleton ();
  gfloat         w, h;
  gint           screen_width, screen_height;

  /* FIXME -- do an animation */
  if (!mnb_alttab_overlay_populate (overlay))
    return FALSE;

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  /*
   * Must ensure size, otherewise the reported actor size is not acurate.
   */
  mx_stylable_style_changed (MX_STYLABLE (overlay), MX_STYLE_CHANGED_FORCE);

  clutter_actor_get_size ((ClutterActor*) overlay, &w, &h);

  clutter_actor_set_position ((ClutterActor*) overlay,
                              (screen_width - w)/2, (screen_height - h)/2);

  mnb_alttab_reset_autoscroll (overlay, backward);
  clutter_actor_show ((ClutterActor*)overlay);

  return TRUE;
}

static void
mnb_alttab_stop_autoscroll (MnbAlttabOverlay *overlay)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (overlay)->priv;

  if (priv->autoscroll_trigger_id)
    {
      g_source_remove (priv->autoscroll_trigger_id);
      priv->autoscroll_trigger_id = 0;
    }

  if (priv->autoscroll_advance_id)
    {
      g_source_remove (priv->autoscroll_advance_id);
      priv->autoscroll_advance_id = 0;
    }
}

void
mnb_alttab_reset_autoscroll (MnbAlttabOverlay *overlay, gboolean backward)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (overlay)->priv;

  priv->backward = backward;

  mnb_alttab_stop_autoscroll (overlay);

  priv->autoscroll_trigger_id =
    g_timeout_add (AUTOSCROLL_TRIGGER_TIMEOUT,
                   mnb_alttab_overlay_autoscroll_trigger_cb,
                   overlay);
}

void
mnb_alttab_overlay_hide (MnbAlttabOverlay *overlay)
{
  MnbAlttabOverlayPrivate *priv = MNB_ALTTAB_OVERLAY (overlay)->priv;

  mnb_alttab_stop_autoscroll (overlay);

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
                                    MutterWindow     *activate,
                                    guint             timestamp)
{
  MnbAlttabOverlayPrivate *priv = overlay->priv;
  MetaWindow              *next;
  MetaWorkspace           *workspace;
  MetaWorkspace           *active_workspace;
  MetaScreen              *screen;

  priv->in_alt_grab = FALSE;

  next = mutter_window_get_meta_window (activate);

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
