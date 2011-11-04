
/*
 * Copyright (c) 2010 Intel Corporation.
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
 * Author: Rob Staudinger <robsta@linux.intel.com>
 */

#ifndef MPL_APP_LAUNCH_CONTEXT_H
#define MPL_APP_LAUNCH_CONTEXT_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define MPL_TYPE_APP_LAUNCH_CONTEXT mpl_app_launch_context_get_type()

#define MPL_APP_LAUNCH_CONTEXT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPL_TYPE_APP_LAUNCH_CONTEXT, MplAppLaunchContext))

#define MPL_APP_LAUNCH_CONTEXT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPL_TYPE_APP_LAUNCH_CONTEXT, MplAppLaunchContextClass))

#define MPL_IS_APP_LAUNCH_CONTEXT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPL_TYPE_APP_LAUNCH_CONTEXT))

#define MPL_IS_APP_LAUNCH_CONTEXT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPL_TYPE_APP_LAUNCH_CONTEXT))

#define MPL_APP_LAUNCH_CONTEXT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPL_TYPE_APP_LAUNCH_CONTEXT, MplAppLaunchContextClass))

typedef struct
{
  GAppLaunchContext parent;
} MplAppLaunchContext;

typedef struct
{
  GAppLaunchContextClass parent;
} MplAppLaunchContextClass;

GType
mpl_app_launch_context_get_type (void);

GAppLaunchContext *
mpl_app_launch_context_new (void);

GAppLaunchContext *
mpl_app_launch_context_get_default (void);

G_END_DECLS

#endif /* MPL_APP_LAUNCH_CONTEXT_H */

