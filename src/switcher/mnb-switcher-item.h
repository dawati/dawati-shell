/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Moblin Netbook
 * Copyright © 2009, Intel Corporation.
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

#ifndef _MNB_SWITCHER_ITEM
#define _MNB_SWITCHER_ITEM

#include <nbtk/nbtk.h>
#include "mnb-switcher.h"

G_BEGIN_DECLS

/*
 * MnbSwitcherItem
 *
 * An abstract NbtkBin subclass represening a single navigable item in the switcher.
 *
 */
#define MNB_TYPE_SWITCHER_ITEM                 (mnb_switcher_item_get_type ())
#define MNB_SWITCHER_ITEM(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_SWITCHER_ITEM, MnbSwitcherItem))
#define MNB_IS_SWITCHER_ITEM(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_SWITCHER_ITEM))
#define MNB_SWITCHER_ITEM_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_SWITCHER_ITEM, MnbSwitcherItemClass))
#define MNB_IS_SWITCHER_ITEM_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_SWITCHER_ITEM))
#define MNB_SWITCHER_ITEM_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_SWITCHER_ITEM, MnbSwitcherItemClass))

typedef struct _MnbSwitcherItem        MnbSwitcherItem;
typedef struct _MnbSwitcherItemPrivate MnbSwitcherItemPrivate;
typedef struct _MnbSwitcherItemClass   MnbSwitcherItemClass;

struct _MnbSwitcherItem
{
  /*< private >*/
  NbtkBin parent_instance;

  MnbSwitcherItemPrivate *priv;
};

struct _MnbSwitcherItemClass
{
  /*< private >*/
  NbtkBinClass parent_class;

  const gchar * (*active_style) (MnbSwitcherItem *item);
  gboolean      (*activate)     (MnbSwitcherItem *item);
};

GType mnb_switcher_item_get_type (void);

MnbSwitcher     *mnb_switcher_item_get_switcher (MnbSwitcherItem *self);
void             mnb_switcher_item_show_tooltip (MnbSwitcherItem *self);
void             mnb_switcher_item_hide_tooltip (MnbSwitcherItem *self);
void             mnb_switcher_item_set_active   (MnbSwitcherItem *self,
                                                 gboolean         active);
gboolean         mnb_switcher_item_is_active    (MnbSwitcherItem *self);
void             mnb_switcher_item_set_tooltip  (MnbSwitcherItem *self,
                                                 const gchar     *text);
MnbSwitcherZone *mnb_switcher_item_get_zone     (MnbSwitcherItem *self);
gboolean         mnb_switcher_item_activate     (MnbSwitcherItem *self);

G_END_DECLS

#endif /* _MNB_SWITCHER_ITEM */
