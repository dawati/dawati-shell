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

#ifndef _MNB_SWITCHER_ZONE_APPS
#define _MNB_SWITCHER_ZONE_APPS

#include "mnb-switcher-zone.h"
#include "mnb-switcher-app.h"

/*
 * MnbSwitcherZone subclass representing a single application zone.
 */
G_BEGIN_DECLS

#define MNB_TYPE_SWITCHER_ZONE_APPS                 (mnb_switcher_zone_apps_get_type ())
#define MNB_SWITCHER_ZONE_APPS(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_SWITCHER_ZONE_APPS, MnbSwitcherZoneApps))
#define MNB_IS_SWITCHER_ZONE_APPS(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_SWITCHER_ZONE_APPS))
#define MNB_SWITCHER_ZONE_APPS_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_SWITCHER_ZONE_APPS, MnbSwitcherZoneAppsClass))
#define MNB_IS_SWITCHER_ZONE_APPS_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_SWITCHER_ZONE_APPS))
#define MNB_SWITCHER_ZONE_APPS_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_SWITCHER_ZONE_APPS, MnbSwitcherZoneAppsClass))

typedef struct _MnbSwitcherZoneApps         MnbSwitcherZoneApps;
typedef struct _MnbSwitcherZoneAppsPrivate  MnbSwitcherZoneAppsPrivate;
typedef struct _MnbSwitcherZoneAppsClass    MnbSwitcherZoneAppsClass;

struct _MnbSwitcherZoneApps
{
  /*< private >*/
  MnbSwitcherZone parent_instance;

  MnbSwitcherZoneAppsPrivate *priv;
};

struct _MnbSwitcherZoneAppsClass
{
  /*< private >*/
  MnbSwitcherZoneClass parent_class;
};

GType mnb_switcher_zone_apps_get_type (void);

MnbSwitcherZoneApps *mnb_switcher_zone_apps_new      (MnbSwitcher *switcher,
                                                      gboolean     active,
                                                      gint         index);
void                 mnb_switcher_zone_apps_populate (MnbSwitcherZoneApps *zone);
MnbSwitcherItem     *mnb_switcher_zone_apps_activate_window (MnbSwitcherZoneApps *zone, MutterWindow *mcw);

G_END_DECLS

#endif /* _MNB_SWITCHER_ZONE_APPS */
