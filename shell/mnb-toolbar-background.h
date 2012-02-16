/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2012 Intel Corp.
 *
 * Authors: Lionel Landwerlin <lionel.g.landwerlin@linux.intel.com>
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


#ifndef _MNB_TOOLBAR_BACKGROUND_H
#define _MNB_TOOLBAR_BACKGROUND_H

#include <glib-object.h>

#include <mx/mx.h>

#include "mnb-toolbar.h"

G_BEGIN_DECLS

#define MNB_TYPE_TOOLBAR_BACKGROUND mnb_toolbar_background_get_type()

#define MNB_TOOLBAR_BACKGROUND(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MNB_TYPE_TOOLBAR_BACKGROUND, MnbToolbarBackground))

#define MNB_TOOLBAR_BACKGROUND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MNB_TYPE_TOOLBAR_BACKGROUND, MnbToolbarBackgroundClass))

#define MNB_IS_TOOLBAR_BACKGROUND(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MNB_TYPE_TOOLBAR_BACKGROUND))

#define MNB_IS_TOOLBAR_BACKGROUND_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MNB_TYPE_TOOLBAR_BACKGROUND))

#define MNB_TOOLBAR_BACKGROUND_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MNB_TYPE_TOOLBAR_BACKGROUND, MnbToolbarBackgroundClass))

typedef struct _MnbToolbarBackground MnbToolbarBackground;
typedef struct _MnbToolbarBackgroundClass MnbToolbarBackgroundClass;
typedef struct _MnbToolbarBackgroundPrivate MnbToolbarBackgroundPrivate;

struct _MnbToolbarBackground
{
  MxBoxLayout parent;

  MnbToolbarBackgroundPrivate *priv;
};

struct _MnbToolbarBackgroundClass
{
  MxBoxLayoutClass parent_class;
};

GType mnb_toolbar_background_get_type (void) G_GNUC_CONST;

ClutterActor *mnb_toolbar_background_new (MnbToolbar *toolbar);

G_END_DECLS

#endif /* _MNB_TOOLBAR_BACKGROUND_H */
