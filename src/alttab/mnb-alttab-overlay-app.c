/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Moblin Netbook
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
#include <string.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "mnb-alttab-overlay.h"
#include "mnb-alttab-overlay-app.h"
#include "../moblin-netbook.h"

#define MNB_SWICHER_APP_ICON_PADDING         5.0
#define MNB_SWICHER_APP_ICON_SIZE           32.0
#define MNB_ALTTAB_OVERLAY_APP_WM_DELETE_TIMEOUT 150

static void mnb_alttab_overlay_app_origin_weak_notify (gpointer, GObject *);

struct _MnbAlttabOverlayAppPrivate
{
  MutterWindow *mcw;     /* MutterWindow we represent */
  ClutterActor *child;
  ClutterActor *icon;
  ClutterActor *tooltip;

  gboolean      disposed : 1; /* disposed guard   */
  gboolean      active   : 1;
};

enum
{
  PROP_0 = 0,

  PROP_MUTTER_WINDOW
};

G_DEFINE_TYPE (MnbAlttabOverlayApp, mnb_alttab_overlay_app, MX_TYPE_BIN);

#define MNB_ALTTAB_OVERLAY_APP_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_ALTTAB_OVERLAY_APP,\
                                MnbAlttabOverlayAppPrivate))

static void
mnb_alttab_overlay_app_dispose (GObject *object)
{
  MnbAlttabOverlayAppPrivate *priv = MNB_ALTTAB_OVERLAY_APP (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;
  priv->child = NULL;

  if (priv->tooltip)
    {
      clutter_actor_unparent (priv->tooltip);
      priv->tooltip = NULL;
    }

  g_object_weak_unref (G_OBJECT (priv->mcw),
                       mnb_alttab_overlay_app_origin_weak_notify, object);

  G_OBJECT_CLASS (mnb_alttab_overlay_app_parent_class)->dispose (object);
}

static void
mnb_alttab_overlay_app_set_property (GObject      *gobject,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  MnbAlttabOverlayAppPrivate *priv = MNB_ALTTAB_OVERLAY_APP (gobject)->priv;

  switch (prop_id)
    {
    case PROP_MUTTER_WINDOW:
      priv->mcw = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_alttab_overlay_app_get_property (GObject    *gobject,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  MnbAlttabOverlayAppPrivate *priv = MNB_ALTTAB_OVERLAY_APP (gobject)->priv;

  switch (prop_id)
    {
    case PROP_MUTTER_WINDOW:
      g_value_set_object (value, priv->mcw);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

/*
 * We need to track the life span of the associated window.
 */
static void
mnb_alttab_overlay_app_origin_weak_notify (gpointer data, GObject *obj)
{
  ClutterActor *self = data;

  /*
   * The original MutterWindow destroyed, destroy self.
   */
  clutter_actor_destroy (self);
}

static void
mnb_alttab_overlay_app_constructed (GObject *self)
{
  ClutterActor          *actor     = CLUTTER_ACTOR (self);
  MnbAlttabOverlayAppPrivate *priv = MNB_ALTTAB_OVERLAY_APP (self)->priv;
  MetaWindow            *meta_win  = mutter_window_get_meta_window (priv->mcw);
  const gchar           *title     = meta_window_get_title (meta_win);
  ClutterActor          *texture, *c_tx;
  GdkPixbuf             *pixbuf = NULL;

  if (G_OBJECT_CLASS (mnb_alttab_overlay_app_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_alttab_overlay_app_parent_class)->constructed (self);

  g_object_get (meta_win, "icon", &pixbuf, NULL);
  if (pixbuf)
    {
      ClutterActor *icon = clutter_texture_new ();

      clutter_texture_set_from_rgb_data (CLUTTER_TEXTURE (icon),
                                         gdk_pixbuf_get_pixels (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf),
                                         gdk_pixbuf_get_width (pixbuf),
                                         gdk_pixbuf_get_height (pixbuf),
                                         gdk_pixbuf_get_rowstride (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf) ? 4 : 3,
                                         0, NULL);

      clutter_actor_set_parent (icon, actor);
      clutter_actor_show (icon);

      priv->icon = icon;
    }

  /*
   * Clone the glx texture in the MutterWindow, and insert it into ourselves.
   */
  texture = mutter_window_get_texture (priv->mcw);
  g_object_set (texture, "keep-aspect-ratio", TRUE, NULL);

  c_tx = priv->child = clutter_clone_new (texture);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), c_tx);
  clutter_actor_set_reactive (actor, TRUE);

  /*
   * Use the window title for tooltip
   */
  if (title)
    {
      priv->tooltip  = g_object_new (MX_TYPE_TOOLTIP, NULL);
      mx_stylable_set_style_class (MX_STYLABLE (priv->tooltip), "alttab");
      clutter_actor_set_parent (priv->tooltip, actor);

      mx_tooltip_set_label (MX_TOOLTIP (priv->tooltip), title);
    }

  g_object_weak_ref (G_OBJECT (priv->mcw),
                     mnb_alttab_overlay_app_origin_weak_notify, self);
}

static void
mnb_alttab_overlay_app_allocate (ClutterActor          *actor,
                                 const ClutterActorBox *box,
                                 ClutterAllocationFlags flags)
{
  MnbAlttabOverlayAppPrivate *priv = MNB_ALTTAB_OVERLAY_APP (actor)->priv;
  MxPadding                   padding    = { 0, };
  gfloat                      parent_width;
  gfloat                      parent_height;

  /*
   * Let the parent class do it's thing, and then allocate for the icon.
   */
  CLUTTER_ACTOR_CLASS (mnb_alttab_overlay_app_parent_class)->allocate (actor,
                                                                 box, flags);
  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  parent_width  = box->x2 - box->x1;
  parent_height = box->y2 - box->y1;

  if (priv->child)
    {
      gfloat          child_w, child_h;
      gdouble         scale, scale_x, scale_y;
      gfloat          available_w, available_h;
      ClutterActorBox allocation = { 0, };

      available_w = parent_width - padding.left - padding.right;
      available_h = parent_height - padding.top - padding.bottom;

      clutter_actor_get_preferred_size (priv->child, NULL, NULL,
                                        &child_w, &child_h);

      scale_x = (gdouble)available_w / (gdouble)child_w;
      scale_y = (gdouble)available_h / (gdouble)child_h;

      scale = scale_x < scale_y ? scale_x : scale_y;

      if (scale > 1.0)
        scale = 1.0;

      child_w = (gfloat)((gdouble)child_w * scale);
      child_h = (gfloat)((gdouble)child_h * scale);

      allocation.x1 = (parent_width - child_w) / 2.0;
      allocation.y1 = (parent_height - child_h) / 2.0;
      allocation.x2 = allocation.x1 + child_w;
      allocation.y2 = allocation.y1 + child_h;

      clutter_actor_allocate (priv->child, &allocation, flags);
    }

  if (priv->icon)
    {
      ClutterActorBox allocation = { 0, };
      gfloat          natural_width, natural_height;
      gfloat          min_width, min_height;
      gfloat          width, height;

      clutter_actor_get_preferred_size (priv->icon,
                                        &min_width,
                                        &min_height,
                                        &natural_width,
                                        &natural_height);

      width  = MNB_SWICHER_APP_ICON_SIZE;
      height = MNB_SWICHER_APP_ICON_SIZE;

      allocation.x2 = parent_width - padding.right -
        MNB_SWICHER_APP_ICON_PADDING;
      allocation.x1 = allocation.x2 - width;
      allocation.y2 = parent_height - padding.bottom -
        MNB_SWICHER_APP_ICON_PADDING;
      allocation.y1 = allocation.y2 - height;

      clutter_actor_allocate (priv->icon, &allocation, flags);
    }

  if (priv->tooltip)
    {
      ClutterVertex    in_v, out_v;
      ClutterGeometry  area;

      in_v.x = in_v.y = in_v.z = 0;

      clutter_actor_apply_transform_to_point (actor, &in_v, &out_v);
      area.x = out_v.x;
      area.y = out_v.y;

      in_v.x = box->x2 - box->x1;
      in_v.y = box->y2 - box->y1;

      clutter_actor_apply_transform_to_point (actor, &in_v, &out_v);
      area.width = out_v.x - area.x;
      area.height = out_v.y - area.y;

      mx_tooltip_set_tip_area ((MxTooltip*) priv->tooltip, &area);

      clutter_actor_allocate_preferred_size (priv->tooltip, flags);
    }
}

static void
mnb_alttab_overlay_app_show (ClutterActor *self)
{
  MnbAlttabOverlayAppPrivate *priv = MNB_ALTTAB_OVERLAY_APP (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_alttab_overlay_app_parent_class)->show (self);

  if (priv->active && priv->tooltip)
    {
      /*
       * This sucks, but the initial allocation of the tooltip is borked
       * (see also the comment in MxWidget about this).
       */
      gfloat          x, y, width, height;
      ClutterGeometry area;

      clutter_actor_get_transformed_position (self, &x, &y);
      clutter_actor_get_size (self, &width, &height);

      area.x = x;
      area.y = y;
      area.width = width;
      area.height = height;

      mx_tooltip_set_tip_area ((MxTooltip*)priv->tooltip, &area);
      mx_tooltip_show ((MxTooltip*)priv->tooltip);
    }

}

static void
mnb_alttab_overlay_app_map (ClutterActor *self)
{
  MnbAlttabOverlayAppPrivate *priv = MNB_ALTTAB_OVERLAY_APP (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_alttab_overlay_app_parent_class)->map (self);

  if (priv->icon)
    clutter_actor_map (priv->icon);

  if (priv->tooltip)
    clutter_actor_map (priv->tooltip);
}

static void
mnb_alttab_overlay_app_unmap (ClutterActor *self)
{
  MnbAlttabOverlayAppPrivate *priv = MNB_ALTTAB_OVERLAY_APP (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_alttab_overlay_app_parent_class)->unmap (self);

  if (priv->icon)
    clutter_actor_unmap (priv->icon);

  if (priv->tooltip)
    clutter_actor_unmap (priv->tooltip);
}

static void
mnb_alttab_overlay_app_pick (ClutterActor *self, const ClutterColor *color)
{
  MnbAlttabOverlayAppPrivate *priv = MNB_ALTTAB_OVERLAY_APP (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_alttab_overlay_app_parent_class)->pick (self,
                                                                     color);

  if (priv->icon && CLUTTER_ACTOR_IS_MAPPED (priv->icon))
    clutter_actor_paint (priv->icon);
}

static void
mnb_alttab_overlay_app_paint (ClutterActor *self)
{
  MnbAlttabOverlayAppPrivate *priv = MNB_ALTTAB_OVERLAY_APP (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_alttab_overlay_app_parent_class)->paint (self);

  if (priv->icon && CLUTTER_ACTOR_IS_MAPPED (priv->icon))
    clutter_actor_paint (priv->icon);

  if (priv->tooltip && CLUTTER_ACTOR_IS_MAPPED (priv->tooltip))
    clutter_actor_paint (priv->tooltip);
}

static void
mnb_alttab_overlay_app_get_preferred_width (ClutterActor *self,
                                            gfloat        for_height,
                                            gfloat       *min_width_p,
                                            gfloat       *natural_width_p)
{
  if (min_width_p)
    *min_width_p = MNB_ALTTAB_OVERLAY_TILE_WIDTH;

  if (natural_width_p)
    *natural_width_p = MNB_ALTTAB_OVERLAY_TILE_WIDTH;
}

static void
mnb_alttab_overlay_app_get_preferred_height (ClutterActor *self,
                                             gfloat        for_width,
                                             gfloat       *min_height_p,
                                             gfloat       *natural_height_p)
{
  if (min_height_p)
    *min_height_p = MNB_ALTTAB_OVERLAY_TILE_HEIGHT;

  if (natural_height_p)
    *natural_height_p = MNB_ALTTAB_OVERLAY_TILE_HEIGHT;
}

static void
mnb_alttab_overlay_app_class_init (MnbAlttabOverlayAppClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass    *actor_class  = CLUTTER_ACTOR_CLASS (klass);

  object_class->dispose             = mnb_alttab_overlay_app_dispose;
  object_class->get_property        = mnb_alttab_overlay_app_get_property;
  object_class->set_property        = mnb_alttab_overlay_app_set_property;
  object_class->constructed         = mnb_alttab_overlay_app_constructed;

  actor_class->allocate             = mnb_alttab_overlay_app_allocate;
  actor_class->paint                = mnb_alttab_overlay_app_paint;
  actor_class->pick                 = mnb_alttab_overlay_app_pick;
  actor_class->show                 = mnb_alttab_overlay_app_show;
  actor_class->map                  = mnb_alttab_overlay_app_map;
  actor_class->unmap                = mnb_alttab_overlay_app_unmap;
  actor_class->get_preferred_width  = mnb_alttab_overlay_app_get_preferred_width;
  actor_class->get_preferred_height = mnb_alttab_overlay_app_get_preferred_height;

  g_type_class_add_private (klass, sizeof (MnbAlttabOverlayAppPrivate));

  g_object_class_install_property (object_class,
                                   PROP_MUTTER_WINDOW,
                                   g_param_spec_object ("mutter-window",
                                                        "Mutter Window",
                                                        "Mutter Window",
                                                        MUTTER_TYPE_COMP_WINDOW,
                                                        G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

static void
mnb_alttab_overlay_app_init (MnbAlttabOverlayApp *self)
{
  MnbAlttabOverlayAppPrivate *priv;

  priv = self->priv = MNB_ALTTAB_OVERLAY_APP_GET_PRIVATE (self);
}

MnbAlttabOverlayApp *
mnb_alttab_overlay_app_new (MutterWindow *mw)
{
  return g_object_new (MNB_TYPE_ALTTAB_OVERLAY_APP,
                       "mutter-window", mw,
                       NULL);
}

void
mnb_alttab_overlay_app_set_active (MnbAlttabOverlayApp *app,
                                   gboolean               active)
{
  MnbAlttabOverlayAppPrivate *priv = MNB_ALTTAB_OVERLAY_APP (app)->priv;
  gboolean not_active = !priv->active;


  if (active && not_active)
    {
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (app), "active");

      if (priv->tooltip)
        mx_tooltip_show ((MxTooltip*)priv->tooltip);
    }
  else if (!active && !not_active)
    {
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (app), NULL);

      if (priv->tooltip)
        mx_tooltip_hide ((MxTooltip*)priv->tooltip);
    }

  priv->active = active;
}

gboolean
mnb_alttab_overlay_app_get_active (MnbAlttabOverlayApp *app)
{
  MnbAlttabOverlayAppPrivate *priv = MNB_ALTTAB_OVERLAY_APP (app)->priv;

  return priv->active;
}

MutterWindow *
mnb_alttab_overlay_app_get_mcw (MnbAlttabOverlayApp *app)
{
  MnbAlttabOverlayAppPrivate *priv = MNB_ALTTAB_OVERLAY_APP (app)->priv;

  return priv->mcw;
}

