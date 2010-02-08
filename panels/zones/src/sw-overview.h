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

#ifndef _SW_OVERVIEW_H
#define _SW_OVERVIEW_H

#include <mx/mx.h>
#include "sw-window.h"

G_BEGIN_DECLS

#define SW_TYPE_OVERVIEW sw_overview_get_type()

#define SW_OVERVIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  SW_TYPE_OVERVIEW, SwOverview))

#define SW_OVERVIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  SW_TYPE_OVERVIEW, SwOverviewClass))

#define SW_IS_OVERVIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  SW_TYPE_OVERVIEW))

#define SW_IS_OVERVIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  SW_TYPE_OVERVIEW))

#define SW_OVERVIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  SW_TYPE_OVERVIEW, SwOverviewClass))

typedef struct _SwOverview SwOverview;
typedef struct _SwOverviewClass SwOverviewClass;
typedef struct _SwOverviewPrivate SwOverviewPrivate;

struct _SwOverview
{
  MxBoxLayout parent;

  SwOverviewPrivate *priv;
};

struct _SwOverviewClass
{
  MxBoxLayoutClass parent_class;
};

GType sw_overview_get_type (void) G_GNUC_CONST;

ClutterActor *sw_overview_new (gint n_zones);

void sw_overview_add_window (SwOverview *overview, SwWindow *window, gint index);
void sw_overview_set_focused_zone (SwOverview *overview, gint index);
void sw_overview_set_focused_window (SwOverview *overview, gulong xid);

void sw_overview_remove_window (SwOverview *overview, gulong xid);

void sw_overview_add_zone (SwOverview *self);

G_END_DECLS

#endif /* _SW_OVERVIEW_H */
