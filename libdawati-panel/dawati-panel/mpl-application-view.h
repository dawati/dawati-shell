/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2012 Intel Corporation.
 *
 * Author: Lionel Landwerlin <lionel.g.landwerlin@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MPL_APPLICATION_VIEW_H
#define _MPL_APPLICATION_VIEW_H

#include <glib-object.h>

#include <mx/mx.h>

G_BEGIN_DECLS

#define MPL_TYPE_APPLICATION_VIEW mpl_application_view_get_type()

#define MPL_APPLICATION_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MPL_TYPE_APPLICATION_VIEW, MplApplicationView))

#define MPL_APPLICATION_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MPL_TYPE_APPLICATION_VIEW, MplApplicationViewClass))

#define MPL_IS_APPLICATION_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MPL_TYPE_APPLICATION_VIEW))

#define MPL_IS_APPLICATION_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MPL_TYPE_APPLICATION_VIEW))

#define MPL_APPLICATION_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MPL_TYPE_APPLICATION_VIEW, MplApplicationViewClass))

typedef struct _MplApplicationView MplApplicationView;
typedef struct _MplApplicationViewClass MplApplicationViewClass;
typedef struct _MplApplicationViewPrivate MplApplicationViewPrivate;

struct _MplApplicationView
{
  MxTable parent;

  MplApplicationViewPrivate *priv;
};

struct _MplApplicationViewClass
{
  MxTableClass parent_class;
};

GType mpl_application_view_get_type (void) G_GNUC_CONST;

ClutterActor *mpl_application_view_new (void);

void mpl_application_view_set_icon (MplApplicationView *view,
                                    ClutterActor       *icon);

void mpl_application_view_set_title (MplApplicationView *view,
                                     const gchar        *text);

void mpl_application_view_set_subtitle (MplApplicationView *view,
                                        const gchar        *text);

void mpl_application_view_set_can_close (MplApplicationView *view,
                                         gboolean            can_close);

void mpl_application_view_set_thumbnail (MplApplicationView *view,
                                         ClutterActor       *thumbnail);

G_END_DECLS

#endif /* _MPL_APPLICATION_VIEW_H */
