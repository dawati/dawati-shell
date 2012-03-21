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
#include <string.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "mnb-alttab-overlay.h"
#include "mnb-alttab-overlay-app.h"

static void mnb_alttab_overlay_app_origin_weak_notify (gpointer, GObject *);

struct _MnbAlttabOverlayAppPrivate
{
  MetaWindowActor *mcw;     /* MetaWindowActor we represent */

  gboolean      disposed : 1; /* disposed guard   */
  gboolean      active   : 1;
};

enum
{
  PROP_0 = 0,

  PROP_MUTTER_WINDOW,
  PROP_BACKGROUND
};

G_DEFINE_TYPE (MnbAlttabOverlayApp, mnb_alttab_overlay_app, MPL_TYPE_APPLICATION_VIEW);

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
      g_object_weak_ref (G_OBJECT (priv->mcw),
                         mnb_alttab_overlay_app_origin_weak_notify, gobject);
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
mnb_alttab_overlay_app_class_init (MnbAlttabOverlayAppClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose             = mnb_alttab_overlay_app_dispose;
  object_class->get_property        = mnb_alttab_overlay_app_get_property;
  object_class->set_property        = mnb_alttab_overlay_app_set_property;

  g_type_class_add_private (klass, sizeof (MnbAlttabOverlayAppPrivate));

  g_object_class_install_property (object_class,
                                   PROP_MUTTER_WINDOW,
                                   g_param_spec_object ("mutter-window",
                                                        "Mutter Window",
                                                        "Mutter Window",
                                                        META_TYPE_WINDOW_ACTOR,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
mnb_alttab_overlay_app_init (MnbAlttabOverlayApp *self)
{
  self->priv = MNB_ALTTAB_OVERLAY_APP_GET_PRIVATE (self);
}

MnbAlttabOverlayApp *
mnb_alttab_overlay_app_new (MetaWindowActor *mcw)
{
  MetaWindow   *mw;
  GdkPixbuf    *pixbuf = NULL;
  ClutterActor *icon = NULL, *texture, *thumbnail;

  g_return_val_if_fail (META_IS_WINDOW_ACTOR (mcw), NULL);

  mw = meta_window_actor_get_meta_window (mcw);

  texture = meta_window_actor_get_texture (mcw);
  clutter_texture_set_keep_aspect_ratio (CLUTTER_TEXTURE (texture),
                                         TRUE);
  thumbnail = clutter_clone_new (texture);


  g_object_get (mw, "icon", &pixbuf, NULL);

  if (pixbuf)
    {
      icon = clutter_texture_new ();
      clutter_texture_set_from_rgb_data (CLUTTER_TEXTURE (icon),
                                         gdk_pixbuf_get_pixels (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf),
                                         gdk_pixbuf_get_width (pixbuf),
                                         gdk_pixbuf_get_height (pixbuf),
                                         gdk_pixbuf_get_rowstride (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf) ? 4 : 3,
                                         0, NULL);

      g_object_unref (pixbuf);
    }

  return g_object_new (MNB_TYPE_ALTTAB_OVERLAY_APP,
                       "mutter-window", mcw,
                       "icon", icon,
                       "title", meta_window_get_description (mw),
                       "subtitle", meta_window_get_title (mw),
                       "thumbnail", thumbnail,
                       "can-close", FALSE,
                       NULL);
}

void
mnb_alttab_overlay_app_set_active (MnbAlttabOverlayApp *app,
                                   gboolean             active)
{
  MnbAlttabOverlayAppPrivate *priv;
  gboolean not_active;

  g_return_if_fail (MNB_IS_ALTTAB_OVERLAY_APP (app));

  priv = app->priv;
  not_active = !priv->active;

  if (active && not_active)
    {
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (app), "active");
    }
  else if (!active && !not_active)
    {
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (app), NULL);
    }

  priv->active = active;
}

gboolean
mnb_alttab_overlay_app_get_active (MnbAlttabOverlayApp *app)
{
  g_return_val_if_fail (MNB_IS_ALTTAB_OVERLAY_APP (app), FALSE);

  return app->priv->active;
}

MetaWindowActor *
mnb_alttab_overlay_app_get_mcw (MnbAlttabOverlayApp *app)
{
  g_return_val_if_fail (MNB_IS_ALTTAB_OVERLAY_APP (app), NULL);

  return app->priv->mcw;
}
