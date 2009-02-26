/*
 * Copyright (C) 2009 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Robert Staudinger <robsta@openedhand.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mnb-launcher-searchbar.h"

#define MNB_LAUNCHER_SEARCHBAR_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_LAUNCHER_SEARCHBAR, MnbLauncherSearchbarPrivate))

struct _MnbLauncherSearchbarPrivate
{
  int foo; /* Placeholder. */
};

G_DEFINE_TYPE (MnbLauncherSearchbar, mnb_launcher_searchbar, NBTK_TYPE_WIDGET)

static void
mnb_launcher_searchbar_class_init (MnbLauncherSearchbarClass *klass)
{
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbLauncherSearchbarPrivate));
}

static void
mnb_launcher_searchbar_init (MnbLauncherSearchbar *self)
{
  self->priv = MNB_LAUNCHER_SEARCHBAR_GET_PRIVATE (self);
}

NbtkWidget *
mnb_launcher_searchbar_new (void)
{
  return g_object_new (MNB_TYPE_LAUNCHER_SEARCHBAR, NULL);
}
