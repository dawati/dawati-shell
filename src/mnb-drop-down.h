/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
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

#ifndef _MNB_DROP_DOWN
#define _MNB_DROP_DOWN

#include <glib-object.h>
#include <clutter/clutter.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

/* useful macro, probably ought to be defined somewhere more generic */
#define MNB_PADDING(a, b, c, d) {CLUTTER_UNITS_FROM_INT (a), CLUTTER_UNITS_FROM_INT (b), \
                                 CLUTTER_UNITS_FROM_INT (c), CLUTTER_UNITS_FROM_INT (d) }
#define MNB_TYPE_DROP_DOWN mnb_drop_down_get_type()

#define MNB_DROP_DOWN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_DROP_DOWN, MnbDropDown))

#define MNB_DROP_DOWN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_DROP_DOWN, MnbDropDownClass))

#define MNB_IS_DROP_DOWN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_DROP_DOWN))

#define MNB_IS_DROP_DOWN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_DROP_DOWN))

#define MNB_DROP_DOWN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_DROP_DOWN, MnbDropDownClass))

typedef struct _MnbDropDownPrivate MnbDropDownPrivate;

typedef struct {
  NbtkTable parent;
  /*< private >*/
  MnbDropDownPrivate *priv;
} MnbDropDown;

typedef struct {
  NbtkTableClass parent_class;

  void (*show_completed) (MnbDropDown *drop_down);
  void (*hide_begin)     (MnbDropDown *drop_down);
  void (*hide_completed) (MnbDropDown *drop_down);
} MnbDropDownClass;

GType mnb_drop_down_get_type (void);

NbtkWidget* mnb_drop_down_new (void);

void          mnb_drop_down_set_child (MnbDropDown *drop_down, ClutterActor *child);
ClutterActor* mnb_drop_down_get_child (MnbDropDown *drop_down);


void          mnb_drop_down_set_button (MnbDropDown *drop_down, NbtkButton *button);

gboolean      mnb_drop_down_should_panel_hide (MnbDropDown *drop_down);

G_END_DECLS

#endif /* _MNB_DROP_DOWN */


