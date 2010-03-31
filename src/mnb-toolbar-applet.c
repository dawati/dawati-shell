/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-toolbar-icon.c */
/*
 * Copyright (c) 2010 Intel Corp.
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

#include <glib/gi18n.h>

#include "mnb-toolbar-applet.h"
#include "mnb-toolbar.h"

G_DEFINE_TYPE (MnbToolbarApplet, mnb_toolbar_applet, MNB_TYPE_TOOLBAR_BUTTON)


#define MNB_TOOLBAR_APPLET_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_TOOLBAR_APPLET, MnbToolbarAppletPrivate))

struct _MnbToolbarAppletPrivate
{
  gboolean disposed : 1;
};

static void
mnb_toolbar_applet_dispose (GObject *object)
{
  MnbToolbarAppletPrivate *priv = MNB_TOOLBAR_APPLET (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  G_OBJECT_CLASS (mnb_toolbar_applet_parent_class)->dispose (object);
}

static void
mnb_toolbar_applet_get_preferred_width (ClutterActor *self,
                                        gfloat        for_height,
                                        gfloat       *min_width_p,
                                        gfloat       *natural_width_p)
{
  gfloat width;

  width = TRAY_BUTTON_WIDTH + TRAY_BUTTON_INTERNAL_PADDING;

  if (min_width_p)
    *min_width_p = width;

  if (natural_width_p)
    *natural_width_p = width;
}

static void
mnb_toolbar_applet_get_preferred_height (ClutterActor *self,
                                        gfloat        for_width,
                                        gfloat       *min_height_p,
                                        gfloat       *natural_height_p)
{
  gfloat height;

  height = TRAY_BUTTON_HEIGHT;

  if (min_height_p)
    *min_height_p = height;

  if (natural_height_p)
    *natural_height_p = height;
}

static void
mnb_toolbar_applet_class_init (MnbToolbarAppletClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbToolbarAppletPrivate));

  object_class->dispose             = mnb_toolbar_applet_dispose;
  actor_class->get_preferred_width  = mnb_toolbar_applet_get_preferred_width;
  actor_class->get_preferred_height = mnb_toolbar_applet_get_preferred_height;
}

static void
mnb_toolbar_applet_init (MnbToolbarApplet *self)
{
  self->priv = MNB_TOOLBAR_APPLET_GET_PRIVATE (self);
}

ClutterActor*
mnb_toolbar_applet_new (void)
{
  return g_object_new (MNB_TYPE_TOOLBAR_APPLET, NULL);
}

