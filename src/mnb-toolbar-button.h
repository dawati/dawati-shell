/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-toolbar-button.h */
/*
 * Copyright (c) 2009 Intel Corp.
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

#ifndef _MNB_TOOLBAR_BUTTON
#define _MNB_TOOLBAR_BUTTON

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MNB_TYPE_TOOLBAR_BUTTON mnb_toolbar_button_get_type()

#define MNB_TOOLBAR_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_TOOLBAR_BUTTON, MnbToolbarButton))

#define MNB_TOOLBAR_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_TOOLBAR_BUTTON, MnbToolbarButtonClass))

#define MNB_IS_TOOLBAR_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_TOOLBAR_BUTTON))

#define MNB_IS_TOOLBAR_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_TOOLBAR_BUTTON))

#define MNB_TOOLBAR_BUTTON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_TOOLBAR_BUTTON, MnbToolbarButtonClass))

typedef struct _MnbToolbarButtonPrivate MnbToolbarButtonPrivate;

typedef struct {
  MxButton parent;

  /*< private >*/
  MnbToolbarButtonPrivate *priv;
} MnbToolbarButton;

typedef struct {
  MxButtonClass parent_class;
} MnbToolbarButtonClass;

GType mnb_toolbar_button_get_type (void);

ClutterActor* mnb_toolbar_button_new (void);
void mnb_toolbar_button_set_reactive_area (MnbToolbarButton  *button, gint x, gint y, gint width, gint height);

G_END_DECLS

#endif /* _MNB_TOOLBAR_BUTTON */

