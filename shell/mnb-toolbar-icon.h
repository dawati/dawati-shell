/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-toolbar-icon.h */
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

#ifndef _MNB_TOOLBAR_ICON
#define _MNB_TOOLBAR_ICON

#include <glib-object.h>

#include "mnb-toolbar-button.h"

G_BEGIN_DECLS

#define MNB_TYPE_TOOLBAR_ICON mnb_toolbar_icon_get_type()

#define MNB_TOOLBAR_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_TOOLBAR_ICON, MnbToolbarIcon))

#define MNB_TOOLBAR_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_TOOLBAR_ICON, MnbToolbarIconClass))

#define MNB_IS_TOOLBAR_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_TOOLBAR_ICON))

#define MNB_IS_TOOLBAR_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_TOOLBAR_ICON))

#define MNB_TOOLBAR_ICON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_TOOLBAR_ICON, MnbToolbarIconClass))

typedef struct _MnbToolbarIconPrivate MnbToolbarIconPrivate;

typedef struct {
  MnbToolbarButton parent;

  /*< private >*/
  MnbToolbarIconPrivate *priv;
} MnbToolbarIcon;

typedef struct {
  MnbToolbarButtonClass parent_class;
} MnbToolbarIconClass;

GType mnb_toolbar_icon_get_type (void);

ClutterActor* mnb_toolbar_icon_new (void);

G_END_DECLS

#endif /* _MNB_TOOLBAR_ICON */

