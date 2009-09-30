/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Thomas Wood <thomas@linux.intel.com>
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

#include "mnb-drop-down.h"
#include "mnb-toolbar.h"    /* For MNB_IS_TOOLBAR */
#include "moblin-netbook.h" /* For PANEL_HEIGHT */
#include "switcher/mnb-switcher.h"

#define SLIDE_DURATION 150

static void mnb_button_toggled_cb (NbtkWidget *, GParamSpec *, MnbDropDown *);

G_DEFINE_TYPE (MnbDropDown, mnb_drop_down, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_DROP_DOWN, MnbDropDownPrivate))

enum {
  PROP_0,

  PROP_MUTTER_PLUGIN,
};

enum
{
  SHOW_BEGIN,
  SHOW_COMPLETED,
  HIDE_BEGIN,
  HIDE_COMPLETED,

  LAST_SIGNAL
};

static guint dropdown_signals[LAST_SIGNAL] = { 0 };

struct _MnbDropDownPrivate {
  MutterPlugin *plugin;

  ClutterActor *child;
  ClutterActor *footer;
  NbtkButton *button;
  gint x;
  gint y;

  guint reparent_cb;

  gboolean in_show_animation : 1;
  gboolean in_hide_animation : 1;
  gboolean hide_toolbar      : 1;

  gulong show_completed_id;
  gulong hide_completed_id;
  ClutterAnimation *show_anim;
  ClutterAnimation *hide_anim;
};

static void
mnb_drop_down_get_property (GObject *object, guint property_id,
                            GValue *value, GParamSpec *pspec)
{
  MnbDropDown *self = MNB_DROP_DOWN (object);

  switch (property_id)
    {
    case PROP_MUTTER_PLUGIN:
      g_value_set_object (value, self->priv->plugin);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_drop_down_set_property (GObject *object, guint property_id,
                            const GValue *value, GParamSpec *pspec)
{
  MnbDropDown *self = MNB_DROP_DOWN (object);

  switch (property_id)
    {
    case PROP_MUTTER_PLUGIN:
      self->priv->plugin = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_drop_down_dispose (GObject *object)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (object)->priv;

  if (priv->button)
    {
      g_signal_handlers_disconnect_by_func (priv->button,
                                            mnb_button_toggled_cb, object);
      priv->button = NULL;
    }

  G_OBJECT_CLASS (mnb_drop_down_parent_class)->dispose (object);
}

static void
mnb_drop_down_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_drop_down_parent_class)->finalize (object);
}

static void
mnb_drop_down_show_completed_cb (ClutterAnimation *anim, ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;

  priv->in_show_animation = FALSE;
  priv->hide_toolbar = FALSE;
  priv->show_anim = NULL;
  priv->show_completed_id = 0;

  if (priv->button)
    {
      if (!nbtk_button_get_checked (priv->button))
        nbtk_button_set_checked (priv->button, TRUE);
    }

  g_signal_emit (actor, dropdown_signals[SHOW_COMPLETED], 0);
  g_object_unref (actor);
}

static void
mnb_toolbar_show_completed_cb (MnbToolbar *toolbar, gpointer data)
{
  ClutterActor *dropdown = CLUTTER_ACTOR (data);

  g_signal_handlers_disconnect_by_func (toolbar,
                                        mnb_toolbar_show_completed_cb,
                                        data);

  clutter_actor_show (dropdown);
}

/*
 * Ensure that the drop down does not stretch over any reserved strut at the
 * bottom of the screen (e.g., if VKB is present)
 */
void
mnb_drop_down_ensure_size (MnbDropDown *self)
{
  MnbDropDownPrivate *priv  = MNB_DROP_DOWN (self)->priv;
  ClutterActor       *actor = CLUTTER_ACTOR (self);

  if (priv->child)
    {
      MetaRectangle  r;
      MetaScreen    *screen;
      MetaWorkspace *workspace;

      screen    = mutter_plugin_get_screen (priv->plugin);
      workspace = meta_screen_get_active_workspace (screen);

      if (workspace)
        {
          gfloat x, y, w, h, wc, hc;
          gint   xi, yi, wi, hi, wci, hci;
          gint   max_height, max_inner_height, inner_width;

          meta_workspace_get_work_area_all_monitors (workspace, &r);

          clutter_actor_get_position (actor, &x, &y);
          clutter_actor_get_size (actor, &w, &h);
          clutter_actor_get_size (priv->child, &wc, &hc);

          xi  = (gint)x;
          yi  = (gint)y;
          wi  = (gint)w;
          hi  = (gint)h;
          wci = (gint)wc;
          hci = (gint)hc;

          /*
           * Maximum height of the panel is the available working height plus the
           * height of the panel shadow (we allow the shaddow to stretch out of
           * the available area).
           */
          max_height = r.y + r.height - yi + MNB_DROP_DOWN_SHADOW_HEIGHT;

          /*
           * inner height is height of the dropdown child, i.e., the max height
           * minus the height of the shadow, minus the height of the footer
           * (half toolbar height), minus the y padding in the panel.
           */
          max_inner_height = max_height - MNB_DROP_DOWN_SHADOW_HEIGHT -
            TOOLBAR_HEIGHT/2 - 4;

          inner_width = r.width - TOOLBAR_X_PADDING * 2;

          /*
           * We have to test the size of the child here, as the external size
           * might not be correct (e.g., when initially showing the Switcher the
           * external size matches the size of the child, i.e., the styling is
           * not yet applied.
           *
           * When initially showing the OOP panels, the child is not yet
           * allocated, and will have size 0; in that case we force the resize
           * otherwise the child would get allocated with the size specified at
           * construction.
           *
           * We rest the size whenever the max height does not match that of the
           * child, so that the window gets resized in both up and down (the
           * height passed into mpl_panel_set_height() is really a maximum
           * allowable height, and if the panel client asked for height lesser
           * than this, the original height request will be respected.
           */
          if (max_inner_height != hci || inner_width != wci)
            {
              if (MNB_IS_PANEL (actor))
                mnb_panel_set_size ((MnbPanel*)actor, r.width, max_height);
              else if (MNB_IS_SWITCHER (actor))
                mnb_switcher_set_size ((MnbSwitcher*)actor,
                                       r.width, max_height);
              else
                clutter_actor_set_size (actor, w, (gfloat) max_height);
            }
        }
    }
}

static void
mnb_drop_down_show (ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;
  gfloat x, y;
  gfloat height, width;
  ClutterAnimation *animation;
  ClutterActor *toolbar;

  if (priv->in_show_animation)
    {
      g_signal_stop_emission_by_name (actor, "show");
      return;
    }

  if (priv->hide_completed_id)
    {
      g_signal_handler_disconnect (priv->hide_anim, priv->hide_completed_id);
      priv->hide_anim = NULL;
      priv->hide_completed_id = 0;
      priv->in_hide_animation = FALSE;
    }

  mnb_drop_down_ensure_size ((MnbDropDown*)actor);

  /*
   * Check the toolbar is visible, if not show it.
   */
  toolbar = clutter_actor_get_parent (actor);
  while (toolbar && !MNB_IS_TOOLBAR (toolbar))
    toolbar = clutter_actor_get_parent (toolbar);

  if (!toolbar)
    {
      g_warning ("Cannot show Panel that is not inside the Toolbar.");
      return;
    }

  if (!CLUTTER_ACTOR_IS_MAPPED (toolbar))
    {
      /*
       * We need to show the toolbar first, and only when it is visible
       * to show this panel.
       */
      g_signal_connect (toolbar, "show-completed",
                        G_CALLBACK (mnb_toolbar_show_completed_cb),
                        actor);

      clutter_actor_show (toolbar);
      return;
    }

  g_signal_emit (actor, dropdown_signals[SHOW_BEGIN], 0);

  CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class)->show (actor);

  clutter_actor_get_position (actor, &x, &y);
  clutter_actor_get_size (actor, &width, &height);

  /* save the size/position so we can clip while we are moving */
  priv->x = x;
  priv->y = y;

  clutter_actor_set_position (actor, x, -height);

  priv->in_show_animation = TRUE;

  g_object_ref (actor);

  animation = clutter_actor_animate (actor, CLUTTER_EASE_IN_SINE,
                                     SLIDE_DURATION,
                                     "x", x,
                                     "y", y,
                                     NULL);

  priv->show_completed_id =
    g_signal_connect_after (animation,
                            "completed",
                            G_CALLBACK (mnb_drop_down_show_completed_cb),
                            actor);
  priv->show_anim = animation;
}

static void
mnb_drop_down_hide_completed_cb (ClutterAnimation *anim, ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;

  priv->hide_anim = NULL;
  priv->hide_completed_id = 0;

  /* the hide animation has finished, so now really hide the actor */
  CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class)->hide (actor);

  /* now that it's hidden we can put it back to where it is suppoed to be */
  clutter_actor_set_position (actor, priv->x, priv->y);

  if (priv->hide_toolbar)
    {
      /*
       * If the hide_toolbar flag is set, we attempt to hide the Toolbar now
       * that the panel is hidden.
       */
      ClutterActor *toolbar = clutter_actor_get_parent (actor);

      while (toolbar && !MNB_IS_TOOLBAR (toolbar))
        toolbar = clutter_actor_get_parent (toolbar);

      if (toolbar)
        mnb_toolbar_hide ((MnbToolbar*)toolbar);

      priv->hide_toolbar = FALSE;
    }

  priv->in_hide_animation = FALSE;
  g_signal_emit (actor, dropdown_signals[HIDE_COMPLETED], 0);
  g_object_unref (actor);
}

static void
mnb_drop_down_hide (ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;
  ClutterAnimation   *animation;

  if (priv->in_hide_animation)
    {
      g_signal_stop_emission_by_name (actor, "hide");
      return;
    }

  priv->in_hide_animation = TRUE;

  if (priv->show_completed_id)
    {
      g_signal_handler_disconnect (priv->show_anim, priv->show_completed_id);
      priv->show_anim = NULL;
      priv->show_completed_id = 0;
      priv->in_show_animation = FALSE;
      priv->hide_toolbar = FALSE;

      if (priv->button)
        {
          if (nbtk_button_get_checked (priv->button))
            nbtk_button_set_checked (priv->button, FALSE);
        }
    }

  g_signal_emit (actor, dropdown_signals[HIDE_BEGIN], 0);

  /* de-activate the button */
  if (priv->button)
    {
      /* hide is hooked into the notify::checked signal from the button, so
       * make sure we don't get into a loop by checking checked first
       */
      if (nbtk_button_get_checked (priv->button))
        nbtk_button_set_checked (priv->button, FALSE);
    }

  if (!priv->child)
    {
      CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class)->hide (actor);
      return;
    }

  g_object_ref (actor);

  animation = clutter_actor_animate (actor, CLUTTER_EASE_IN_SINE,
                                     SLIDE_DURATION,
                                     "y", -clutter_actor_get_height (actor),
                                     NULL);

  priv->hide_completed_id =
    g_signal_connect_after (animation,
                            "completed",
                            G_CALLBACK (mnb_drop_down_hide_completed_cb),
                            actor);

  priv->hide_anim = animation;
}

static void
mnb_drop_down_paint (ClutterActor *actor)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;
  ClutterGeometry geom;

  clutter_actor_get_allocation_geometry (actor, &geom);

  cogl_clip_push (priv->x - geom.x,
                  priv->y - geom.y,
                  geom.width,
                  geom.height);

  CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class)->paint (actor);
  cogl_clip_pop ();
}

static gboolean
mnb_button_event_capture (ClutterActor *actor, ClutterButtonEvent *event)
{
  /* prevent the event from moving up the scene graph, since we don't want
   * any events to accidently fall onto application windows below the
   * drop down.
   */
  return TRUE;
}

static void
mnb_button_toggled_cb (NbtkWidget  *button,
                       GParamSpec  *pspec,
                       MnbDropDown *drop_down)
{
  ClutterActor *actor = CLUTTER_ACTOR (drop_down);

  if (nbtk_button_get_checked (NBTK_BUTTON (button)))
    {
      /*
       * Must reset the y in case a previous animation ended prematurely
       * and the y is not set correctly; see bug 900.
       */
      clutter_actor_set_y (actor, TOOLBAR_HEIGHT);
      clutter_actor_show (actor);
    }
  else
    clutter_actor_hide (actor);
}

static void
mnb_drop_down_allocate (ClutterActor          *actor,
                        const ClutterActorBox *box,
                        ClutterAllocationFlags flags)
{
#if 0
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (actor)->priv;
#endif
  ClutterActorClass  *parent_class;

  /*
   * The show and hide animations trigger allocations with origin_changed
   * set to TRUE; if we call the parent class allocation in this case, it
   * will force relayout, which we do not want. Instead, we call directly the
   * ClutterActor implementation of allocate(); this ensures our actor box is
   * correct, which is all we call about during the animations.
   *
   * If the drop down is not visible, we just return; this insures that the
   * needs_allocation flag in ClutterActor remains set, and the actor will get
   * reallocated when we show it.
   */
  if (!CLUTTER_ACTOR_IS_VISIBLE (actor))
    return;

#if 0
  /*
   * This is not currently reliable, e.g., the Switcher is animating empty.
   * Once we have the binary flags in Clutter 1.0 to differentiate between
   * allocations that are purely due to change of position and the rest, we
   * need to disable this.
   */
  if (priv->in_show_animation || priv->in_hide_animation)
    {
      ClutterActorClass  *actor_class;

      actor_class = g_type_class_peek (CLUTTER_TYPE_ACTOR);

      if (actor_class)
        actor_class->allocate (actor, box, origin_changed);

      return;
    }
#endif

  parent_class = CLUTTER_ACTOR_CLASS (mnb_drop_down_parent_class);
  parent_class->allocate (actor, box, flags);
}

static void
mnb_drop_down_constructed (GObject *object)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (object)->priv;
  NbtkWidget         *footer;

  /* footer with "up" button */
  footer = nbtk_button_new ();
  nbtk_widget_set_style_class_name (footer, "drop-down-footer");
  nbtk_table_add_actor (NBTK_TABLE (object), CLUTTER_ACTOR (footer), 1, 0);
  g_signal_connect_swapped (footer, "clicked",
                            G_CALLBACK (mnb_drop_down_hide_with_toolbar), object);

  priv->footer = CLUTTER_ACTOR (footer);
}

static void
mnb_drop_down_class_init (MnbDropDownClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *clutter_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbDropDownPrivate));

  object_class->get_property = mnb_drop_down_get_property;
  object_class->set_property = mnb_drop_down_set_property;
  object_class->dispose = mnb_drop_down_dispose;
  object_class->finalize = mnb_drop_down_finalize;
  object_class->constructed = mnb_drop_down_constructed;

  clutter_class->show = mnb_drop_down_show;
  clutter_class->hide = mnb_drop_down_hide;
  clutter_class->paint = mnb_drop_down_paint;
  clutter_class->allocate = mnb_drop_down_allocate;
  clutter_class->button_press_event = mnb_button_event_capture;
  clutter_class->button_release_event = mnb_button_event_capture;
  clutter_class->allocate = mnb_drop_down_allocate;

  dropdown_signals[SHOW_BEGIN] =
    g_signal_new ("show-begin",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbDropDownClass, show_begin),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  dropdown_signals[SHOW_COMPLETED] =
    g_signal_new ("show-completed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbDropDownClass, show_completed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  dropdown_signals[HIDE_BEGIN] =
    g_signal_new ("hide-begin",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbDropDownClass, hide_begin),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  dropdown_signals[HIDE_COMPLETED] =
    g_signal_new ("hide-completed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbDropDownClass, hide_completed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_object_class_install_property (object_class,
                                   PROP_MUTTER_PLUGIN,
                                   g_param_spec_object ("mutter-plugin",
                                                      "Mutter Plugin",
                                                      "Mutter Plugin",
                                                      MUTTER_TYPE_PLUGIN,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));
}

static void
mnb_drop_down_init (MnbDropDown *self)
{
  self->priv = GET_PRIVATE (self);

  g_object_set (self,
                "show-on-set-parent", FALSE,
                "reactive", TRUE,
                NULL);
}

NbtkWidget*
mnb_drop_down_new (MutterPlugin *plugin)
{
  return g_object_new (MNB_TYPE_DROP_DOWN,
                       "mutter-plugin", plugin,
                       NULL);
}

static void
mnb_drop_down_reparent_cb (ClutterActor *child,
                           ClutterActor *old_parent,
                           gpointer      data)
{
  MnbDropDownPrivate *priv = MNB_DROP_DOWN (data)->priv;

  if (clutter_actor_get_parent (child) != data)
    {
      if (priv->reparent_cb)
        {
          g_signal_handler_disconnect (priv->child, priv->reparent_cb);
          priv->reparent_cb = 0;
        }

      priv->child = NULL;
    }
}

void
mnb_drop_down_set_child (MnbDropDown *drop_down,
                         ClutterActor *child)
{
  MnbDropDownPrivate *priv;

  g_return_if_fail (MNB_IS_DROP_DOWN (drop_down));
  g_return_if_fail (child == NULL || CLUTTER_IS_ACTOR (child));

  priv = drop_down->priv;

  if (priv->child)
    {
      if (priv->reparent_cb)
        {
          g_signal_handler_disconnect (priv->child, priv->reparent_cb);
          priv->reparent_cb = 0;
        }

      clutter_container_remove_actor (CLUTTER_CONTAINER (drop_down),
                                      priv->child);
    }

  if (child)
    {
      priv->reparent_cb =
        g_signal_connect (child, "parent-set",
                          G_CALLBACK (mnb_drop_down_reparent_cb),
                          drop_down);
      nbtk_table_add_actor (NBTK_TABLE (drop_down), child, 0, 0);
    }

  priv->child = child;
}

ClutterActor*
mnb_drop_down_get_child (MnbDropDown *drop_down)
{
  g_return_val_if_fail (MNB_DROP_DOWN (drop_down), NULL);

  return drop_down->priv->child;
}

static void
mnb_drop_down_button_weak_unref_cb (MnbDropDown *drop_down, GObject *button)
{
  drop_down->priv->button = NULL;
}

void
mnb_drop_down_set_button (MnbDropDown *drop_down,
                          NbtkButton *button)
{
  NbtkButton *old_button;

  g_return_if_fail (MNB_IS_DROP_DOWN (drop_down));
  g_return_if_fail (!button || NBTK_IS_BUTTON (button));

  old_button = drop_down->priv->button;
  drop_down->priv->button = button;

  if (old_button)
    {
      g_object_weak_unref (G_OBJECT (old_button),
                           (GWeakNotify) mnb_drop_down_button_weak_unref_cb,
                           drop_down);

      g_signal_handlers_disconnect_by_func (old_button,
                                            G_CALLBACK (mnb_button_toggled_cb),
                                            drop_down);
    }

  if (button)
    {
      g_object_weak_ref (G_OBJECT (button),
                         (GWeakNotify) mnb_drop_down_button_weak_unref_cb,
                         drop_down);

      g_signal_connect (button,
                        "notify::checked",
                        G_CALLBACK (mnb_button_toggled_cb),
                        drop_down);
    }
}

/*
 * Hides both dropdown and toolbar, in a sequnce so as to preserve the
 * hide animations.
 */
void
mnb_drop_down_hide_with_toolbar (MnbDropDown *self)
{
  MnbDropDownPrivate *priv = self->priv;

  priv->hide_toolbar = TRUE;

  if (priv->in_hide_animation)
    return;

  clutter_actor_hide (CLUTTER_ACTOR (self));
}

/*
 * Returns untransformed geometry of the footer relative to the drop down.
 */
void
mnb_drop_down_get_footer_geometry (MnbDropDown *self,
                                   gfloat      *x,
                                   gfloat      *y,
                                   gfloat      *width,
                                   gfloat      *height)
{
  MnbDropDownPrivate *priv = self->priv;

  g_return_if_fail (x && y && width && height);

  /*
   * ??? Something borked here; if I query coords of the footer I am getting
   * what looks like the unexpanded size of the cell, relative to its column
   * and row position, not to the parent. Work around it.
   */
  *x      = clutter_actor_get_x (CLUTTER_ACTOR (self));
  *y      = clutter_actor_get_height (priv->child);
  *width  = clutter_actor_get_width  (CLUTTER_ACTOR (self));
  *height = clutter_actor_get_height (priv->footer);
}

