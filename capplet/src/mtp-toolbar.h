/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mtp-toolbar.h */
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

#ifndef _MTP_TOOLBAR
#define _MTP_TOOLBAR

#include <mx/mx.h>

G_BEGIN_DECLS

#define MTP_TYPE_TOOLBAR mtp_toolbar_get_type()

#define MTP_TOOLBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MTP_TYPE_TOOLBAR, MtpToolbar))

#define MTP_TOOLBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MTP_TYPE_TOOLBAR, MtpToolbarClass))

#define MTP_IS_TOOLBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MTP_TYPE_TOOLBAR))

#define MTP_IS_TOOLBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MTP_TYPE_TOOLBAR))

#define MTP_TOOLBAR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MTP_TYPE_TOOLBAR, MtpToolbarClass))

typedef struct _MtpToolbarPrivate MtpToolbarPrivate;

typedef struct {
  MxWidget parent;

  /*< private >*/
  MtpToolbarPrivate *priv;
} MtpToolbar;

typedef struct {
  MxWidgetClass parent_class;
} MtpToolbarClass;

GType mtp_toolbar_get_type (void);

ClutterActor* mtp_toolbar_new (void);

void mtp_toolbar_add_button    (MtpToolbar *toolbar, ClutterActor *button);
void mtp_toolbar_remove_button (MtpToolbar *toolbar, ClutterActor *button);
void mtp_toolbar_insert_button (MtpToolbar       *toolbar,
                                ClutterActor     *button,
                                ClutterActor     *before);
gboolean mtp_toolbar_remove_space (MtpToolbar *toolbar, gboolean applet);
void mtp_toolbar_readd_button  (MtpToolbar *toolbar, ClutterActor *button);
gboolean mtp_toolbar_was_modified (MtpToolbar *toolbar);
void     mtp_toolbar_clear_modified_state (MtpToolbar *toolbar);
void     mtp_toolbar_mark_modified (MtpToolbar *toolbar);
GList   *mtp_toolbar_get_panel_buttons (MtpToolbar *toolbar);
GList   *mtp_toolbar_get_applet_buttons (MtpToolbar *toolbar);
void     mtp_toolbar_fill_space (MtpToolbar *toolbar);
gboolean mtp_toolbar_has_free_space (MtpToolbar *toolbar);

G_END_DECLS

#endif /* _MTP_TOOLBAR */

