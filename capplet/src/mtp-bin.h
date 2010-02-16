/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mtp-bin.h */
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

#ifndef _MTP_BIN
#define _MTP_BIN

#include <mx/mx.h>

G_BEGIN_DECLS

#define MTP_TYPE_BIN mtp_bin_get_type()

#define MTP_BIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MTP_TYPE_BIN, MtpBin))

#define MTP_BIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MTP_TYPE_BIN, MtpBinClass))

#define MTP_IS_BIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MTP_TYPE_BIN))

#define MTP_IS_BIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MTP_TYPE_BIN))

#define MTP_BIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MTP_TYPE_BIN, MtpBinClass))

typedef struct _MtpBinPrivate MtpBinPrivate;

typedef struct {
  MxBoxLayout parent;

  /*< private >*/
  MtpBinPrivate *priv;
} MtpBin;

typedef struct {
  MxBoxLayoutClass parent_class;
} MtpBinClass;

GType mtp_bin_get_type (void);

ClutterActor* mtp_bin_new (void);

void mtp_bin_load_contents (MtpBin *bin);

G_END_DECLS

#endif /* _MTP_BIN */

