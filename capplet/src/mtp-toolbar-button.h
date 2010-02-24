/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mtp-toolbar-button.h */
/*
 * Copyright (c) 2009, 2010 Intel Corp.
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

#ifndef _MTP_TOOLBAR_BUTTON
#define _MTP_TOOLBAR_BUTTON

#include <mx/mx.h>

G_BEGIN_DECLS

#define MTP_TYPE_TOOLBAR_BUTTON mtp_toolbar_button_get_type()

#define MTP_TOOLBAR_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MTP_TYPE_TOOLBAR_BUTTON, MtpToolbarButton))

#define MTP_TOOLBAR_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MTP_TYPE_TOOLBAR_BUTTON, MtpToolbarButtonClass))

#define MTP_IS_TOOLBAR_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MTP_TYPE_TOOLBAR_BUTTON))

#define MTP_IS_TOOLBAR_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MTP_TYPE_TOOLBAR_BUTTON))

#define MTP_TOOLBAR_BUTTON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MTP_TYPE_TOOLBAR_BUTTON, MtpToolbarButtonClass))

typedef struct _MtpToolbarButtonPrivate MtpToolbarButtonPrivate;

typedef struct {
  MxWidget parent;

  /*< private >*/
  MtpToolbarButtonPrivate *priv;
} MtpToolbarButton;

typedef struct {
  MxWidgetClass parent_class;
} MtpToolbarButtonClass;

GType mtp_toolbar_button_get_type (void);

ClutterActor* mtp_toolbar_button_new (void);

void mtp_toolbar_button_set_name (MtpToolbarButton *button, const gchar *name);
const gchar *mtp_toolbar_button_get_name (MtpToolbarButton *button);
gboolean mtp_toolbar_button_is_applet (MtpToolbarButton *button);
gboolean mtp_toolbar_button_is_clock (MtpToolbarButton *button);
ClutterActor *mnb_toolbar_button_get_pre_drag_parent (MtpToolbarButton *button);
gboolean mtp_toolbar_button_is_valid (MtpToolbarButton *button);
gboolean mtp_toolbar_button_is_required (MtpToolbarButton *button);
void mtp_toolbar_button_set_dont_pick (MtpToolbarButton *button, gboolean dont);

G_END_DECLS

#endif /* _MTP_TOOLBAR_BUTTON */

