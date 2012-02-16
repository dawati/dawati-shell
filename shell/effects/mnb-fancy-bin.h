/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Chris Lord <chris@linux.intel.com>
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

#ifndef _MNB_FANCY_BIN_H
#define _MNB_FANCY_BIN_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MNB_TYPE_FANCY_BIN mnb_fancy_bin_get_type()

#define MNB_FANCY_BIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MNB_TYPE_FANCY_BIN, MnbFancyBin))

#define MNB_FANCY_BIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MNB_TYPE_FANCY_BIN, MnbFancyBinClass))

#define MNB_IS_FANCY_BIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MNB_TYPE_FANCY_BIN))

#define MNB_IS_FANCY_BIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MNB_TYPE_FANCY_BIN))

#define MNB_FANCY_BIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MNB_TYPE_FANCY_BIN, MnbFancyBinClass))

typedef struct _MnbFancyBin MnbFancyBin;
typedef struct _MnbFancyBinClass MnbFancyBinClass;
typedef struct _MnbFancyBinPrivate MnbFancyBinPrivate;

struct _MnbFancyBin
{
  MxWidget parent;

  MnbFancyBinPrivate *priv;
};

struct _MnbFancyBinClass
{
  MxWidgetClass parent_class;
};

GType mnb_fancy_bin_get_type (void) G_GNUC_CONST;

ClutterActor *mnb_fancy_bin_new ();

void          mnb_fancy_bin_set_child (MnbFancyBin *bin, ClutterActor *child);
ClutterActor *mnb_fancy_bin_get_child (MnbFancyBin *bin);
void          mnb_fancy_bin_set_fancy (MnbFancyBin *bin, gboolean ooh_get_you);
gboolean      mnb_fancy_bin_get_fancy (MnbFancyBin *bin);

G_END_DECLS

#endif /* _MNB_FANCY_BIN_H */
