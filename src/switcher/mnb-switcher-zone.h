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

#ifndef _MNB_SWITCHER_ZONE
#define _MNB_SWITCHER_ZONE

#include "mnb-switcher.h"
#include "mnb-switcher-item.h"

G_BEGIN_DECLS

/*
 * MnbSwitcherZone is n abstract class represening a single zone in the
 * switcher. Actual zone are represented by specialized subclasses, such as
 * MnbSwitcherZoneApps.
 */
#define MNB_TYPE_SWITCHER_ZONE                 (mnb_switcher_zone_get_type ())
#define MNB_SWITCHER_ZONE(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_SWITCHER_ZONE, MnbSwitcherZone))
#define MNB_IS_SWITCHER_ZONE(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_SWITCHER_ZONE))
#define MNB_SWITCHER_ZONE_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_SWITCHER_ZONE, MnbSwitcherZoneClass))
#define MNB_IS_SWITCHER_ZONE_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_SWITCHER_ZONE))
#define MNB_SWITCHER_ZONE_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_SWITCHER_ZONE, MnbSwitcherZoneClass))

typedef enum
  {
    MNB_SWITCHER_ZONE_NORMAL,
    MNB_SWITCHER_ZONE_ACTIVE,
    MNB_SWITCHER_ZONE_HOVER,
  }
  MnbSwitcherZoneState;

/* NB: MnbSwitcherZone is forward-declared in mnb-switcher.h */
typedef struct _MnbSwitcherZonePrivate MnbSwitcherZonePrivate;
typedef struct _MnbSwitcherZoneClass   MnbSwitcherZoneClass;

struct _MnbSwitcherZone
{
  /*< private >*/
  NbtkTable parent_instance;

  MnbSwitcherZonePrivate *priv;
};

struct _MnbSwitcherZoneClass
{
  /*< private >*/
  NbtkTableClass parent_class;

  /*
   * The following functions return the name of the css style
   * that is to be applied to the zone components in given state.
   *
   * Here 'zone'  is the inner area of the object,
   *      'label' is the header area above the zone,
   *      'text'  is the text withing the label.
   */
  const gchar     *(*zone_style)  (MnbSwitcherZone      *zone,
                                   MnbSwitcherZoneState  state);
  const gchar     *(*label_style) (MnbSwitcherZone      *zone,
                                   MnbSwitcherZoneState  state);
  const gchar     *(*text_style)  (MnbSwitcherZone      *zone,
                                   MnbSwitcherZoneState  state);

  /*
   * The following functions return the name of the css class for
   * the inner elements of the zone (see comment above for nomenclature).
   */
  const gchar     *(*zone_class)  (MnbSwitcherZone      *zone);
  const gchar     *(*label_class) (MnbSwitcherZone      *zone);
  const gchar     *(*text_class)  (MnbSwitcherZone      *zone);

  /*
   * For zones that contain navigable items the following fuctions are used
   * to navigate and select items in the zone.
   *
   * These functions must be implemented by all zones that contain navigable
   * items.
   */
  MnbSwitcherItem *(*next_item)   (MnbSwitcherZone      *zone,
                                   MnbSwitcherItem      *current);
  MnbSwitcherItem *(*prev_item)   (MnbSwitcherZone      *zone,
                                   MnbSwitcherItem      *current);
  gboolean         (*select_item) (MnbSwitcherZone      *zone,
                                   MnbSwitcherItem      *item);
  void             (*unselect_all)(MnbSwitcherZone      *zone);

  /*
   * For zones that are pageable, i.e., the zone as a whole represents a single
   * navigable item within the switcher, this function is used to select the
   * zone.
   *
   * This can be implemented by zones that declare themselves pageable and
   * require any special steps to be take on selection. If custom
   * implementation is not provided, the default behaviour is to apply the
   * active state to the zone.
   */
  gboolean         (*select)      (MnbSwitcherZone      *zone);

  /*
   * Implements the activation for the type, e.g., what happens when the user
   * selects the zone with Alt+Tab.
   *
   * All pageable zones must implent this vfunction.
   */
  gboolean         (*activate)    (MnbSwitcherZone      *zone);
};

GType mnb_switcher_zone_get_type   (void);

void             mnb_switcher_zone_set_state        (MnbSwitcherZone      *zone,
                                                     MnbSwitcherZoneState  state);
void             mnb_switcher_zone_reset_state      (MnbSwitcherZone      *zone);
gint             mnb_switcher_zone_get_index        (MnbSwitcherZone      *zone);
void             mnb_switcher_zone_set_index        (MnbSwitcherZone      *zone,
                                                     gint                  index);
gboolean         mnb_switcher_zone_is_active        (MnbSwitcherZone      *zone);
void             mnb_switcher_zone_set_active       (MnbSwitcherZone      *zone,
                                                     gboolean              active);
MnbSwitcherItem *mnb_switcher_zone_get_next_item    (MnbSwitcherZone      *zone,
                                                     MnbSwitcherItem      *current);
MnbSwitcherItem *mnb_switcher_zone_get_prev_item    (MnbSwitcherZone      *zone,
                                                     MnbSwitcherItem      *current);
gboolean         mnb_switcher_zone_select_item      (MnbSwitcherZone      *zone,
                                                     MnbSwitcherItem      *item);
gboolean         mnb_switcher_zone_select           (MnbSwitcherZone      *zone);
void             mnb_switcher_zone_unselect_all     (MnbSwitcherZone      *zone);
MnbSwitcher     *mnb_switcher_zone_get_switcher     (MnbSwitcherZone      *zone);
NbtkTable       *mnb_switcher_zone_get_content_area (MnbSwitcherZone      *zone);
gboolean         mnb_switcher_zone_is_pageable      (MnbSwitcherZone      *zone);
void             mnb_switcher_zone_set_pageable     (MnbSwitcherZone      *zone,
                                                     gboolean              whether);
gboolean         mnb_switcher_zone_has_items        (MnbSwitcherZone      *zone);
void             mnb_switcher_zone_set_has_items    (MnbSwitcherZone      *zone,
                                                     gboolean              whether);
MnbSwitcherItem *mnb_switcher_zone_get_active_item  (MnbSwitcherZone      *zone);
gboolean         mnb_switcher_zone_activate         (MnbSwitcherZone      *zone);

G_END_DECLS

#endif /* _MNB_SWITCHER_ZONE */
