/*
 * Copyright © 2009, 2010, Intel Corporation.
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
 *
 * Authors: Thomas Wood <thomas.wood@intel.com>
 */

#ifndef _SW_ZONE_H
#define _SW_ZONE_H

#include <mx/mx.h>

G_BEGIN_DECLS

#define SW_TYPE_ZONE sw_zone_get_type()

#define SW_ZONE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  SW_TYPE_ZONE, SwZone))

#define SW_ZONE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  SW_TYPE_ZONE, SwZoneClass))

#define SW_IS_ZONE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  SW_TYPE_ZONE))

#define SW_IS_ZONE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  SW_TYPE_ZONE))

#define SW_ZONE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  SW_TYPE_ZONE, SwZoneClass))

typedef struct _SwZone SwZone;
typedef struct _SwZoneClass SwZoneClass;
typedef struct _SwZonePrivate SwZonePrivate;

struct _SwZone
{
  MxWidget parent;

  SwZonePrivate *priv;
};

struct _SwZoneClass
{
  MxWidgetClass parent_class;
};

GType sw_zone_get_type (void) G_GNUC_CONST;

ClutterActor *sw_zone_new (void);
void sw_zone_set_dummy (SwZone *zone, gboolean dummy);
gboolean sw_zone_get_dummy (SwZone *zone);
void sw_zone_set_number (SwZone *zone, gint number);
gint sw_zone_get_number (SwZone *zone);
void sw_zone_set_focused (SwZone *zone, gboolean focused);
void sw_zone_set_focused_window (SwZone *zone, gulong xid);
gboolean sw_zone_remove_window (SwZone *zone, gulong xid);
void sw_zone_set_is_welcome (SwZone *zone, gboolean welcome);

void sw_zone_set_drag_in_progress (SwZone *zone, gboolean drag_in_progress);

G_END_DECLS

#endif /* _SW_ZONE_H */
