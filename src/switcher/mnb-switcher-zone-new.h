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

#ifndef _MNB_SWITCHER_ZONE_NEW
#define _MNB_SWITCHER_ZONE_NEW

#include "mnb-switcher-zone.h"

/*
 * MnbSwitcherZoneNew is a MnbSwitcherZone subclass representing the 'new' zone
 * in the switcher to which applications can be dragged to create new zone.
 */
G_BEGIN_DECLS

#define MNB_TYPE_SWITCHER_ZONE_NEW                 (mnb_switcher_zone_new_get_type ())
#define MNB_SWITCHER_ZONE_NEW(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_SWITCHER_ZONE_NEW, MnbSwitcherZoneNew))
#define MNB_IS_SWITCHER_ZONE_NEW(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_SWITCHER_ZONE_NEW))
#define MNB_SWITCHER_ZONE_NEW_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_SWITCHER_ZONE_NEW, MnbSwitcherZoneNewClass))
#define MNB_IS_SWITCHER_ZONE_NEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_SWITCHER_ZONE_NEW))
#define MNB_SWITCHER_ZONE_NEW_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_SWITCHER_ZONE_NEW, MnbSwitcherZoneNewClass))

typedef struct _MnbSwitcherZoneNew        MnbSwitcherZoneNew;
typedef struct _MnbSwitcherZoneNewPrivate MnbSwitcherZoneNewPrivate;
typedef struct _MnbSwitcherZoneNewClass   MnbSwitcherZoneNewClass;

struct _MnbSwitcherZoneNew
{
  /*< private >*/
  MnbSwitcherZone parent_instance;

  MnbSwitcherZoneNewPrivate *priv;
};

struct _MnbSwitcherZoneNewClass
{
  /*< private >*/
  MnbSwitcherZoneClass parent_class;
};

GType mnb_switcher_zone_new_get_type (void);

MnbSwitcherZone *mnb_switcher_zone_new_new (MnbSwitcher *switcher, gint index);

G_END_DECLS

#endif /* _MNB_SWITCHER_ZONE_NEW */
