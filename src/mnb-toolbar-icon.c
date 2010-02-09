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

#include "mnb-toolbar-icon.h"
#include "mnb-toolbar.h"

G_DEFINE_TYPE (MnbToolbarIcon, mnb_toolbar_icon, MNB_TYPE_TOOLBAR_BUTTON)


#define MNB_TOOLBAR_ICON_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_TOOLBAR_ICON, MnbToolbarIconPrivate))

struct _MnbToolbarIconPrivate
{
  gboolean disposed : 1;
};

static void
mnb_toolbar_icon_dispose (GObject *object)
{
  MnbToolbarIconPrivate *priv = MNB_TOOLBAR_ICON (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;
}

static void
mnb_toolbar_icon_class_init (MnbToolbarIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbToolbarIconPrivate));


  object_class->dispose = mnb_toolbar_icon_dispose;
}

static void
mnb_toolbar_icon_init (MnbToolbarIcon *self)
{
  self->priv = MNB_TOOLBAR_ICON_GET_PRIVATE (self);
}

ClutterActor*
mnb_toolbar_icon_new (void)
{
  return g_object_new (MNB_TYPE_TOOLBAR_ICON, NULL);
}

