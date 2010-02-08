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

#ifndef _MTP_SPACE
#define _MTP_SPACE

#include <mx/mx.h>

G_BEGIN_DECLS

#define MTP_TYPE_SPACE mtp_space_get_type()

#define MTP_SPACE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MTP_TYPE_SPACE, MtpSpace))

#define MTP_SPACE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MTP_TYPE_SPACE, MtpSpaceClass))

#define MTP_IS_SPACE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MTP_TYPE_SPACE))

#define MTP_IS_SPACE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MTP_TYPE_SPACE))

#define MTP_SPACE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MTP_TYPE_SPACE, MtpSpaceClass))

typedef struct _MtpSpacePrivate MtpSpacePrivate;

typedef struct {
  MxWidget parent;

  /*< private >*/
  MtpSpacePrivate *priv;
} MtpSpace;

typedef struct {
  MxWidgetClass parent_class;
} MtpSpaceClass;

GType mtp_space_get_type (void);

ClutterActor* mtp_space_new (void);

void mtp_space_set_dont_pick (MtpSpace *button, gboolean dont);
void mtp_space_set_is_applet (MtpSpace *button, gboolean applet);

G_END_DECLS

#endif /* _MTP_SPACE */

